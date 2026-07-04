#!/usr/bin/env python3
import argparse
import base64
import math
import secrets
import textwrap
from pathlib import Path


PUBLIC_EXPONENT = 65537
KEY_BITS = 1024


def der_len(length):
    if length < 0x80:
        return bytes([length])
    encoded = length.to_bytes((length.bit_length() + 7) // 8, "big")
    return bytes([0x80 | len(encoded)]) + encoded


def der_int(value):
    raw = value.to_bytes((value.bit_length() + 7) // 8 or 1, "big")
    if raw[0] & 0x80:
        raw = b"\0" + raw
    return b"\x02" + der_len(len(raw)) + raw


def der_seq(*items):
    body = b"".join(items)
    return b"\x30" + der_len(len(body)) + body


def pem(label, der):
    payload = base64.b64encode(der).decode("ascii")
    lines = "\n".join(textwrap.wrap(payload, 64))
    return f"-----BEGIN {label}-----\n{lines}\n-----END {label}-----\n"


def is_probable_prime(value):
    if value < 2:
        return False
    small_primes = (
        3, 5, 7, 11, 13, 17, 19, 23, 29, 31,
        37, 41, 43, 47, 53, 59, 61, 67, 71, 73,
    )
    if value == 2:
        return True
    if value % 2 == 0:
        return False
    for prime in small_primes:
        if value == prime:
            return True
        if value % prime == 0:
            return False

    d = value - 1
    s = 0
    while d % 2 == 0:
        s += 1
        d //= 2

    for _ in range(32):
        a = secrets.randbelow(value - 3) + 2
        x = pow(a, d, value)
        if x == 1 or x == value - 1:
            continue
        for _ in range(s - 1):
            x = pow(x, 2, value)
            if x == value - 1:
                break
        else:
            return False
    return True


def generate_prime(bits):
    while True:
        candidate = secrets.randbits(bits)
        candidate |= (1 << (bits - 1)) | 1
        if math.gcd(candidate - 1, PUBLIC_EXPONENT) == 1 and is_probable_prime(candidate):
            return candidate


def generate_rsa_key(bits):
    half = bits // 2
    while True:
        p = generate_prime(half)
        q = generate_prime(bits - half)
        if p == q:
            continue
        n = p * q
        if n.bit_length() != bits:
            continue
        phi = (p - 1) * (q - 1)
        if math.gcd(PUBLIC_EXPONENT, phi) != 1:
            continue
        d = pow(PUBLIC_EXPONENT, -1, phi)
        return {
            "n": n,
            "e": PUBLIC_EXPONENT,
            "d": d,
            "p": p,
            "q": q,
            "dp": d % (p - 1),
            "dq": d % (q - 1),
            "qi": pow(q, -1, p),
        }


def private_pem(key):
    return pem(
        "RSA PRIVATE KEY",
        der_seq(
            der_int(0),
            der_int(key["n"]),
            der_int(key["e"]),
            der_int(key["d"]),
            der_int(key["p"]),
            der_int(key["q"]),
            der_int(key["dp"]),
            der_int(key["dq"]),
            der_int(key["qi"]),
        ),
    )


def public_pem(key):
    return pem(
        "RSA PUBLIC KEY",
        der_seq(
            der_int(key["n"]),
            der_int(key["e"]),
        ),
    )


def cpp_literal(pem_text):
    lines = pem_text.rstrip("\n").splitlines()
    return '"\\\n' + "\\n\\\n".join(lines) + '\\\n"'


def write_private_header(path, private_text):
    literal = cpp_literal(private_text)
    content = (
        "#pragma once\n\n"
        "// ZyeGram update signing key. Keep this file private.\n"
        "const char *PrivateKey = " + literal + ";\n\n"
        "const char *PrivateBetaKey = " + literal + ";\n\n"
        "const char *AlphaPrivateKey = " + literal + ";\n"
    )
    path.write_text(content, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--force", action="store_true", help="overwrite existing keys")
    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    root = script_dir.parents[2]
    private_dir = root / "DesktopPrivate" / "zyegram_updates"
    private_dir.mkdir(parents=True, exist_ok=True)

    private_path = private_dir / "zyegram_update_private.pem"
    public_path = private_dir / "zyegram_update_public.pem"
    header_path = private_dir / "zyegram_packer_private.h"

    if not args.force and (private_path.exists() or public_path.exists() or header_path.exists()):
        raise SystemExit(
            "Keys already exist. Use --force only if you intentionally want to rotate the update key."
        )

    key = generate_rsa_key(KEY_BITS)
    private_text = private_pem(key)
    public_text = public_pem(key)

    private_path.write_text(private_text, encoding="ascii")
    public_path.write_text(public_text, encoding="ascii")
    write_private_header(header_path, private_text)

    print("Private files written to:")
    print(private_dir)
    print()
    print("Public key:")
    print(public_text)


if __name__ == "__main__":
    main()
