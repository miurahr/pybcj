import binascii
import hashlib
import pathlib
import zipfile

import bcj

BLOCKSIZE = 8192


def test_x86_encode_once(tmp_path):
    with zipfile.ZipFile(pathlib.Path(__file__).parent.joinpath("data/src.zip")) as zipsrc:
        src = zipsrc.read("x86_1.bin")
    encoder = bcj.BCJEncoder()
    dest = encoder.encode(src)
    dest += encoder.flush()
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("e396dadbbe0be4190cdea986e0ec949b049ded2b38df19268a78d32b90b72d42")


def test_x86_encode_chunked(tmp_path):
    with zipfile.ZipFile(pathlib.Path(__file__).parent.joinpath("data/src.zip")) as srczip:
        with srczip.open("x86_3.bin") as fin:
            encoder = bcj.BCJEncoder()
            m = hashlib.sha256()
            with open(tmp_path.joinpath("output.bin"), "wb") as fout:
                while True:
                    src = fin.read(BLOCKSIZE)
                    if len(src) == 0:
                        break
                    dest = encoder.encode(src)
                    m.update(dest)
                    fout.write(dest)
                dest = encoder.flush()
                fout.write(dest)
                m.update(dest)
    assert m.digest() == binascii.unhexlify("10b19883b74588706ec888d70f128cf027894c96cf379786b06ad0b47a78f5d1")


def test_x86_decode_once(tmp_path):
    with zipfile.ZipFile(pathlib.Path(__file__).parent.joinpath("data/src.zip")) as zipsrc:
        src = zipsrc.read("bcj_1.bin")
    decoder = bcj.BCJDecoder(len(src))
    dest = decoder.decode(src)
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("5ae0726746e2ccdad8f511ecfcf5f79df4533b83f86b1877cebc07f14a4e9b6a")


def test_x86_decode_huge(tmp_path):
    """
    Test a case to decode a large file.
    :param tmp_path:
    :return:
    """
    with zipfile.ZipFile(pathlib.Path(__file__).parent.joinpath("data/src.zip")) as zipsrc:
        total_length = zipsrc.getinfo("bcj_3.bin").file_size
        remaining = total_length
        decoder = bcj.BCJDecoder(total_length)
        m = hashlib.sha256()
        with zipsrc.open("bcj_3.bin", mode="r") as fin:
            with open(tmp_path.joinpath("output.bin"), "wb") as fout:
                while remaining > 0:
                    src = fin.read(BLOCKSIZE)
                    dest = decoder.decode(src)
                    remaining -= len(dest)
                    m.update(dest)
                    fout.write(dest)
    assert m.digest() == binascii.unhexlify("a0a0fd3c854e68b74f09ba1913b895402aad41b83cb5c691cee66b3c59aa4f10")


def test_x86_decode_huge_variable_chunked(tmp_path):
    """
    Test a case to decode variable sizes as similar as py7zr behaior.
    :param tmp_path:
    :return:
    """
    sizes = [46080, 46080, 35018, 322411, 48640, 48640, 1513556, 997376]
    hashes = [
        "abc6af45e7c983f37d54453a0f1c9cbce08cd590730bc3f38a8da575eb2df91f",
        "abc6af45e7c983f37d54453a0f1c9cbce08cd590730bc3f38a8da575eb2df91f",
        "8306a2b0e9985d5364f092f9ec2e4f84e787c3d7abed4f4735c330de98da5926",
        "4a01689e46572487b895f5297aeef197d14477ec645fe8713eafe3a1af8edf42",
        "73c170b4a80a57a2da3e11237bfe7d10a6359e05f7dbc8584934807f0177c4ba",
        "73c170b4a80a57a2da3e11237bfe7d10a6359e05f7dbc8584934807f0177c4ba",
        "f0e65b92b61426017d443ddf63750259025c3f4fc60d22fa98e81bcfb1f3fdbf",
        "e03f6b389e4c8a49c2a891cfef50eaed1e714a9ffe2deb4e0617568ba0943422",
    ]
    with zipfile.ZipFile(pathlib.Path(__file__).parent.joinpath("data/src.zip")) as zipsrc:
        total_length = zipsrc.getinfo("bcj_3.bin").file_size
        decoder = bcj.BCJDecoder(total_length)
        with zipsrc.open("bcj_3.bin", mode="r") as f:
            for i in range(8):
                m = hashlib.sha256()
                remaining = sizes[i]
                while remaining > 0:
                    src = f.read(min(remaining, BLOCKSIZE))
                    dest = decoder.decode(src)
                    m.update(dest)
                    remaining -= len(dest)
                assert m.digest() == binascii.unhexlify(hashes[i])


def test_x86_encode_decode_once(tmp_path):
    """
    Test a case to encode and decode with single chunk.
    :param tmp_path:
    :return:
    """
    with zipfile.ZipFile(pathlib.Path(__file__).parent.joinpath("data/src.zip")) as f:
        src = f.read("x86_1.bin")
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


def test_x86_encode_decode_chunked(tmp_path):
    """
    Test a case to encode and decode with multiple chunks.
    :param tmp_path:
    :return:
    """
    dest = bytearray()
    encoder = bcj.BCJEncoder()
    with zipfile.ZipFile(pathlib.Path(__file__).parent.joinpath("data/src.zip")) as zipsrc:
        with zipsrc.open("x86_1.bin", mode="r") as f:
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


def test_x86_encode_decode_large(tmp_path):
    """
    Test a case to encode and decode a large binary file.
    :param tmp_path:
    :return:
    """
    with zipfile.ZipFile(pathlib.Path(__file__).parent.joinpath("data/src.zip")) as zipsrc:
        dest = bytearray()
        encoder = bcj.BCJEncoder()
        m = hashlib.sha256()
        with zipsrc.open("x86_3.bin", mode="r") as f:
            src = f.read(BLOCKSIZE)
            while len(src) > 0:
                m.update(src)
                dest += encoder.encode(src)
                src = f.read(BLOCKSIZE)
    hashsrc = m.digest()
    dest += encoder.flush()
    total_length = len(dest)
    #
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    #
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("10b19883b74588706ec888d70f128cf027894c96cf379786b06ad0b47a78f5d1")
    #
    decoder = bcj.BCJDecoder(total_length)
    with open(tmp_path.joinpath("output.bin"), "rb") as f:
        dest = bytearray()
        m = hashlib.sha256()
        remaining = total_length
        while remaining > 0:
            size = min(remaining, BLOCKSIZE)
            src = f.read(size)
            tmp = decoder.decode(src)
            remaining -= len(tmp)
            dest += tmp
        m.update(dest)
        hashresult = m.digest()
    assert hashresult == hashsrc
