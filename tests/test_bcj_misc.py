import binascii
import hashlib
import pathlib

import pytest

import pybcj


def test_aarch64_encode(tmp_path):
    with open(pathlib.Path(__file__).parent.joinpath('data/lib/aarch64-linux-gnu/liblzma.so.0'), 'rb') as f:
        src = f.read()
    encoder = pybcj.ARMEncoder()
    dest = encoder.encode(src)
    dest += encoder.flush()
    with open(tmp_path.joinpath('output.bin'), 'wb') as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify('be4b1217015838b417a255a3bb1d17ec8a9357c0e195b00bcd5b48959aac8295')


@pytest.mark.skip(reason="not implemented yet.")
def test_ppc_encode(tmp_path):
    with open(pathlib.Path(__file__).parent.joinpath('data/lib/powerpc64le-linux-gnu/liblzma.so.0'), 'rb') as f:
        src = f.read()
    encoder = pybcj.PPCEncoder()
    dest = encoder.encode(src)
    dest += encoder.flush()
    with open(tmp_path.joinpath('output.bin'), 'wb') as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify('e396dadbbe0be4190cdea986e0ec949b049ded2b38df19268a78d32b90b72d42')
