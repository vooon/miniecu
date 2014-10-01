# -*- python -*-

from sqlalchemy import Column, DateTime, String, Integer, BigInteger, LargeBinary, \
    Enum, ForeignKey, func
from sqlalchemy.orm import relationship, backref
from sqlalchemy.ext.declarative import declarative_base


Base = declarative_base()


class Log(Base):
    """
    Log table.

    Store logs information
    """
    __tablename__ = 'log'
    id = Column(Integer, primary_key=True)
    start_date = Column(DateTime(timezone=True), nullable=False, default=func.now())
    name = Column(String(255, convert_unicode=True))
    source = Column(String(255, convert_unicode=True))


class PBTag(Base):
    """
    PB Tag table.

    Stores miniecu.Message tag_id -> type
    """
    __tablename__ = 'pb_tag'
    id = Column(Integer, primary_key=True, autoincrement=False)
    message_type = Column(String(50, convert_unicode=True), nullable=False)
    field = Column(String(50, convert_unicode=True), nullable=False)


class LogData(Base):
    """
    Log Data table.

    Stores log items.
    """
    __tablename__ = 'log_data'
    id = Column(Integer, primary_key=True)
    log_id = Column(Integer, ForeignKey('log.id'), nullable=False)
    sys_date = Column(DateTime(timezone=True), nullable=False, default=func.now())
    direction = Column(Enum("SEND", "RECV", name="log_direction"), nullable=False)
    dev_time_ms = Column(BigInteger, doc='ecu timestamp [ms]')
    engine_id = Column(Integer, doc='ecu address')
    pb_tag_id = Column(Integer, ForeignKey('pb_tag.id'), nullable=True)
    pb_message = Column(LargeBinary, nullable=False)

    log = relationship(
        Log,
        backref=backref('log', uselist=True, cascade='delete,all'))

    pb_tag = relationship(
        PBTag,
        backref=backref('pb_tag'))


from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
import miniecu_pb2 as msgs


DIR_SEND = True
DIR_RECV = False


class Logger(object):
    """
    This class creates log and then append data to it.
    """
    def __init__(self, conn_url):
        self.engine = create_engine(conn_url)
        self.Session = sessionmaker()
        self.Session.configure(bind=self.engine)
        Base.metadata.create_all(self.engine)

    def update_pb_tags(self):
        s = self.Session()
        s.query(PBTag).delete()
        for tag_id, message_type, field in \
                ((k, v.message_type.full_name, v.full_name) for k, v in
                 msgs.Message.DESCRIPTOR.fields_by_number.iteritems()):
            s.add(PBTag(id=tag_id, message_type=message_type, field=field))

        s.commit()

    def start(self, name=None, source=None, start_date=None):
        self.update_pb_tags()   # XXX: possible loss of data in prev logs
        self.log = Log(name=name, source=source, start_date=start_date)
        self.session = self.Session()
        self.session.add(self.log)
        self.session.commit()

    def add_message(self, msg, direction, sys_date=None):
        if not isinstance(msg, msgs.Message):
            raise ValueError("unknown type")
        elif not hasattr(self, 'log'):
            raise RuntimeError("no log instance, did you call start()?")

        engine_id = None
        timestamp_ms = None
        pb_tag_id = None
        direction_ = 'SEND' if direction == DIR_SEND else 'RECV'

        fields = msg.ListFields()
        if len(fields) > 1:
            pass    # TODO: add warning about incorrect message

        if len(fields) > 0:
            desc, data = fields[0]
            pb_tag_id = desc.number
            if hasattr(data, 'engine_id'):      engine_id = data.engine_id
            if hasattr(data, 'timestamp_ms'):   timestamp_ms = data.timestamp_ms

        self.session.add(
            LogData(log=self.log, sys_date=sys_date, direction=direction_,
                    dev_time_ms=timestamp_ms, engine_id=engine_id,
                    pb_tag_id=pb_tag_id, pb_message=msg.SerializeToString()))
        self.session.commit()


class LoggingWrapper(object):
    def __init__(self, pbstx, logger):
        self.pbstx = pbstx
        self.logger = logger

    def send(self, msg):
        self.pbstx.send(msg)
        self.logger.add_message(msg, DIR_SEND)

    def receive(self):
        msg = self.pbstx.receive()
        self.logger.add_message(msg, DIR_RECV)
        return msg
