# -*- python -*-


def pb_to_kv_pairs(pb, prefix=()):
    for desc, val in pb.ListFields():
        if isinstance(val, (int, float, bool, long, basestring)):
            yield '.'.join(prefix + (desc.name, )), val
        else:
            for pair in pb_to_kv_pairs(val, prefix + (desc.name, )):
                yield pair


def _status(val):
    return 'TODO %s' % val


STATUS_STRINGIFYER = {
    'status': _status,
}


def status_str(field, val):
    return STATUS_STRINGIFYER.get(field, str)(val)
