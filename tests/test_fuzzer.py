import io
from datetime import timedelta

from hypothesis import given, settings
from hypothesis import strategies as st

import bcj


@given(obj=st.binary(min_size=1, max_size=10240000), bufsize=st.integers(min_value=8192, max_value=1024000))
@settings(deadline=timedelta(milliseconds=300))
def test_fuzzer(obj, bufsize):
    encoder = bcj.BCJEncoder()
    length = len(obj)
    filtered = bytearray()
    f = io.BytesIO(obj)
    data = f.read(bufsize)
    while len(data) > 0:
        filtered += encoder.encode(obj)
        data = f.read(bufsize)
    filtered += encoder.flush()
    decoder = bcj.BCJDecoder(length)
    result = bytearray()
    f = io.BytesIO(filtered)
    data = f.read(bufsize)
    while len(data) > 0:
        result += decoder.decode(filtered)
        data = f.read(bufsize)
    assert result == obj
