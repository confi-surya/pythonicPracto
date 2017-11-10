from urllib import quote as _quote
import itertools
import codecs
utf8_decoder = codecs.getdecoder('utf-8')
utf8_encoder = codecs.getencoder('utf-8')


def get_valid_utf8_str(str_or_unicode):
    """
    Get valid parts of utf-8 str from str, unicode and even invalid utf-8 str

    :param str_or_unicode: a string or an unicode which can be invalid utf-8
    """
    if isinstance(str_or_unicode, unicode):
        (str_or_unicode, _len) = utf8_encoder(str_or_unicode, 'replace')
    (valid_utf8_str, _len) = utf8_decoder(str_or_unicode, 'replace')
    return valid_utf8_str.encode('utf-8')

def quote(value, safe='/'):
    """
    Patched version of urllib.quote that encodes utf-8 strings before quoting
    """
    return _quote(get_valid_utf8_str(value), safe)

def split_path(path, minsegs=1, maxsegs=None, rest_with_last=False):
    """
    Validate and split the given HTTP request path.

    **Examples**::

        ['a'] = split_path('/a')
        ['a', None] = split_path('/a', 1, 2)
        ['a', 'c'] = split_path('/a/c', 1, 2)
        ['a', 'c', 'o/r'] = split_path('/a/c/o/r', 1, 3, True)

    :param path: HTTP Request path to be split
    :param minsegs: Minimum number of segments to be extracted
    :param maxsegs: Maximum number of segments to be extracted
    :param rest_with_last: If True, trailing data will be returned as part
                           of last segment.  If False, and there is
                           trailing data, raises ValueError.
    :returns: list of segments with a length of maxsegs (non-existant
              segments will return as None)
    :raises: ValueError if given an invalid path
    """
    if not maxsegs:
        maxsegs = minsegs
    if minsegs > maxsegs:
        raise ValueError('minsegs > maxsegs: %d > %d' % (minsegs, maxsegs))
    if rest_with_last:
        segs = path.split('/', maxsegs)
        minsegs += 1
        maxsegs += 1
        count = len(segs)
        if (segs[0] or count < minsegs or count > maxsegs or
                '' in segs[1:minsegs]):
            raise ValueError('Invalid path: %s' % quote(path))
    else:
        minsegs += 1
        maxsegs += 1
        segs = path.split('/', maxsegs)
        count = len(segs)
        if (segs[0] or count < minsegs or count > maxsegs + 1 or
                '' in segs[1:minsegs] or
                (count == maxsegs + 1 and segs[maxsegs])):
            raise ValueError('Invalid path: %s' % quote(path))
    segs = segs[1:maxsegs]
    segs.extend([None] * (maxsegs - 1 - len(segs)))
    return segs


class CloseableChain(object):
    """
    Like itertools.chain, but with a close method that will attempt to invoke
    its sub-iterators' close methods, if any.
    """
    def __init__(self, *iterables):
        self.iterables = iterables

    def __iter__(self):
        return iter(itertools.chain(*(self.iterables)))

    def close(self):
        for it in self.iterables:
            close_method = getattr(it, 'close', None)
            if close_method:
                close_method()


def reiterate(iterable):
    """
    Consume the first item from an iterator, then re-chain it to the rest of
    the iterator.  This is useful when you want to make sure the prologue to
    downstream generators have been executed before continuing.

    :param iterable: an iterable object
    """
    if isinstance(iterable, (list, tuple)):
        return iterable
    else:
        iterator = iter(iterable)
        try:
            chunk = ''
            while not chunk:
                chunk = next(iterator)
            return CloseableChain([chunk], iterator)
        except StopIteration:
            return []

def float_comp(a, b, rel_prec=1e-8):
    if abs(a-b) <= rel_prec * (max(abs(a), abs(b)) if a != 0 != b else 1):
        return 0
    return a-b
