# -*- python -*-

import time
from comm import msgs


def pb_to_kv_pairs(pb, prefix=()):
    for desc, val in pb.ListFields():
        if isinstance(val, (int, float, bool, long, basestring)):
            yield '.'.join(prefix + (desc.name, )), val
        else:
            for pair in pb_to_kv_pairs(val, prefix + (desc.name, )):
                yield pair


def _status(val):
    flags = []

    for flag, name in (
        (msgs.Status.ARMED, 'ARMED'),
        (msgs.Status.TIME_KNOWN, 'TIME'),
        (msgs.Status.IGNITION_ENABLED, 'IGNITION'),
        (msgs.Status.STARTER_ENABLED, 'STARTER'),
        (msgs.Status.ENGINE_RUNNING, 'RUNNING'),

        (msgs.Status.ERROR, 'ERROR'),
        (msgs.Status.UNDERVOLTAGE, 'UNDERVOLTAGE'),
        (msgs.Status.OVERHEAT, 'OVERHEAT'),
        (msgs.Status.LOW_FUEL, 'LOW-FUEL'),
        (msgs.Status.HIGH_RPM, 'HIGH-RPM'),
    ):
        if val & flag:
            flags.append(name)

    return '{}: {}'.format(val, '|'.join(flags))

def _timestamp_ms(val):
    val /= 1000.0
    if val > 1234567890:
        return '{}: {}'.format(val, time.ctime(val))
    else:
        return str(val)


STATUS_STRINGIFYER = {
    'status': _status,
    'timestamp_ms': _timestamp_ms,
}


def status_str(field, val):
    return STATUS_STRINGIFYER.get(field, str)(val)
