import binascii
import hashlib
import pathlib

import bcj


def test_x86_encode(tmp_path):
    with open(pathlib.Path(__file__).parent.joinpath('data/x86.bin'), 'rb') as f:
        src = f.read()
    encoder = bcj.BCJEncoder()
    dest = encoder.encode(src)
    dest += encoder.flush()
    with open(tmp_path.joinpath('output.bin'), 'wb') as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify('e396dadbbe0be4190cdea986e0ec949b049ded2b38df19268a78d32b90b72d42')


def test_x86_decode(tmp_path):
    with open(pathlib.Path(__file__).parent.joinpath('data/bcj.bin'), 'rb') as f:
        src = f.read()
    decoder = bcj.BCJDecoder(len(src))
    dest = decoder.decode(src)
    with open(tmp_path.joinpath('output.bin'), 'wb') as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify('5ae0726746e2ccdad8f511ecfcf5f79df4533b83f86b1877cebc07f14a4e9b6a')
