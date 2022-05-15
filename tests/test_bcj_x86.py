import binascii
import hashlib
import pathlib

import bcj

BLOCKSIZE = 8192


def test_x86_encode(tmp_path):
    with open(pathlib.Path(__file__).parent.joinpath("data/x86.bin"), "rb") as f:
        src = f.read()
    encoder = bcj.BCJEncoder()
    dest = encoder.encode(src)
    dest += encoder.flush()
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("e396dadbbe0be4190cdea986e0ec949b049ded2b38df19268a78d32b90b72d42")


def test_x86_decode(tmp_path):
    with open(pathlib.Path(__file__).parent.joinpath("data/bcj.bin"), "rb") as f:
        src = f.read()
    decoder = bcj.BCJDecoder(len(src))
    dest = decoder.decode(src)
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("5ae0726746e2ccdad8f511ecfcf5f79df4533b83f86b1877cebc07f14a4e9b6a")


def test_x86_encode_decode1(tmp_path):
    with open(pathlib.Path(__file__).parent.joinpath("data/x86.bin"), "rb") as f:
        src = f.read()
    encoder = bcj.BCJEncoder()
    dest = encoder.encode(src)
    dest += encoder.flush()
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("e396dadbbe0be4190cdea986e0ec949b049ded2b38df19268a78d32b90b72d42")
    with open(tmp_path.joinpath("output.bin"), "rb") as f:
        src = f.read()
    decoder = bcj.BCJDecoder(len(src))
    dest = decoder.decode(src)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("5ae0726746e2ccdad8f511ecfcf5f79df4533b83f86b1877cebc07f14a4e9b6a")


def test_x86_encode_decode2(tmp_path):
    dest = bytearray()
    encoder = bcj.BCJEncoder()
    with open(pathlib.Path(__file__).parent.joinpath("data/x86.bin"), "rb") as f:
        src = f.read(BLOCKSIZE)
        while len(src) > 0:
            dest += encoder.encode(src)
            src = f.read(BLOCKSIZE)
    dest += encoder.flush()
    assert len(dest) == 12800
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("e396dadbbe0be4190cdea986e0ec949b049ded2b38df19268a78d32b90b72d42")
    #
    dest = bytearray()
    decoder = bcj.BCJDecoder(12800)
    with open(tmp_path.joinpath("output.bin"), "rb") as f:
        src = f.read(BLOCKSIZE)
        while len(src) > 0:
            dest += decoder.decode(src)
            src = f.read(BLOCKSIZE)
    m = hashlib.sha256(dest)
    assert m.digest() == binascii.unhexlify("5ae0726746e2ccdad8f511ecfcf5f79df4533b83f86b1877cebc07f14a4e9b6a")


def test_x86_encode_decode_complex(tmp_path):
    hashsrc = list()
    hashresult = list()
    exe_path = pathlib.Path(__file__).parent.joinpath("data/x86")
    dest = bytearray()
    encoder = bcj.BCJEncoder()
    for i in range(8):
        m = hashlib.sha256()
        refpath = exe_path.joinpath(f"{i}.exe")
        with open(refpath, "rb") as f:
            src = f.read(BLOCKSIZE)
            while len(src) > 0:
                m.update(src)
                dest += encoder.encode(src)
                src = f.read(BLOCKSIZE)
        hashsrc.append(m.digest())
    dest += encoder.flush()
    total_length = len(dest)
    #
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    #
    decoder = bcj.BCJDecoder(total_length)
    with open(tmp_path.joinpath("output.bin"), "rb") as f:
        for i in range(8):
            refpath = exe_path.joinpath(f"{i}.exe")
            dest = bytearray()
            m = hashlib.sha256()
            remaining = refpath.stat().st_size
            while remaining > 0:
                size = min(remaining, BLOCKSIZE)
                src = f.read(size)
                tmp = decoder.decode(src)
                remaining -= len(tmp)
                dest += tmp
            m.update(dest)
            hashresult.append(m.digest())
    for i in range(8):
        assert hashresult[i] == hashsrc[i], f"filter error in {i}.exe"
