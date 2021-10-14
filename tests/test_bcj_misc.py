import binascii
import hashlib
import pathlib

import bcj


def test_aarch64_encode(tmp_path):
    with open(
        pathlib.Path(__file__).parent.joinpath("data/lib/aarch64-linux-gnu/liblzma.so.0"),
        "rb",
    ) as f:
        src = f.read()
    encoder = bcj.ARMEncoder()
    dest = encoder.encode(src)
    dest += encoder.flush()
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("be4b1217015838b417a255a3bb1d17ec8a9357c0e195b00bcd5b48959aac8295")


def test_ppc_encode(tmp_path):
    with open(
        pathlib.Path(__file__).parent.joinpath("data/lib/powerpc64le-linux-gnu/liblzma.so.0"),
        "rb",
    ) as f:
        src = f.read()
    encoder = bcj.PPCEncoder()
    dest = encoder.encode(src)
    dest += encoder.flush()
    with open(tmp_path.joinpath("output.bin"), "wb") as f:
        f.write(dest)
    m = hashlib.sha256()
    m.update(dest)
    assert m.digest() == binascii.unhexlify("0289683dfa366682f0d6cc17880ed64b1f98ab889bc62dff0b9098bcdd8d4af3")
