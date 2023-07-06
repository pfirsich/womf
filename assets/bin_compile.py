import argparse
import struct
from enum import Enum
from dataclasses import dataclass
from typing import Any, Callable


class ByteOrder(Enum):
    Native = "native"
    LittleEndian = "le"
    BigEndian = "be"


byte_order_format = {
    ByteOrder.Native: "@",
    ByteOrder.LittleEndian: "<",
    ByteOrder.BigEndian: ">",
}


@dataclass
class DataType:
    identifier: str
    fmt: str
    constructor: Callable


int_constructor = lambda s: int(s, base=0)

data_types = [
    DataType("i8", "b", int_constructor),
    DataType("u8", "B", int_constructor),
    DataType("i16", "h", int_constructor),
    DataType("u16", "H", int_constructor),
    DataType("i32", "i", int_constructor),
    DataType("u32", "I", int_constructor),
    DataType("i64", "q", int_constructor),
    DataType("u64", "Q", int_constructor),
    DataType("f32", "f", float),
]


@dataclass
class Value:
    byte_order: ByteOrder
    data_type: DataType
    value: Any


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("infile")
    parser.add_argument("outfile", nargs="?")
    args = parser.parse_args()

    valid_byte_orders = set(e.name for e in ByteOrder)
    id_type_map = {t.identifier: t for t in data_types}

    current_type = None
    current_byte_order = ByteOrder.Native
    values: list[Value] = []
    with open(args.infile, "r") as f:
        for line in f:
            pre_comment = line.split("#", maxsplit=1)[0]
            parts = pre_comment.split()
            for part in parts:
                if part in id_type_map:
                    current_type = id_type_map[part]
                elif part in valid_byte_orders:
                    current_byte_order = ByteOrder(part)
                else:
                    assert current_type != None
                    values.append(
                        Value(
                            current_byte_order,
                            current_type,
                            current_type.constructor(part),
                        )
                    )

    outfile = args.outfile
    if outfile == None:
        outfile = args.infile + ".bin"
    print(outfile)

    with open(outfile, "wb") as f:
        for value in values:
            fmt = byte_order_format[value.byte_order] + value.data_type.fmt
            f.write(struct.pack(fmt, value.value))


if __name__ == "__main__":
    main()
