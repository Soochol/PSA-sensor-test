"""
CRC-8 CCITT unit tests.

Tests verify the Python implementation matches the firmware lookup table.
"""

import pytest
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol.crc import CRC8
from psa_protocol.constants import Command


@pytest.mark.unit
class TestCRC8:
    """Test CRC-8 CCITT implementation."""

    def test_empty_data(self):
        """CRC of empty data should be 0."""
        assert CRC8.calculate(b'') == 0

    def test_single_byte_zero(self):
        """CRC of single byte 0x00."""
        # table[0 ^ 0] = table[0] = 0x00
        assert CRC8.calculate(bytes([0x00])) == 0x00

    def test_single_byte_one(self):
        """CRC of single byte 0x01."""
        # table[0 ^ 1] = table[1] = 0x07
        assert CRC8.calculate(bytes([0x01])) == 0x07

    def test_single_byte_ff(self):
        """CRC of single byte 0xFF."""
        # table[0 ^ 0xFF] = table[255] = 0xF3
        assert CRC8.calculate(bytes([0xFF])) == 0xF3

    def test_ping_frame_crc(self):
        """Test CRC calculation for PING command frame."""
        # PING frame: LEN=0, CMD=0x01
        # CRC covers: [0x00, 0x01]
        crc_data = bytes([0x00, Command.PING])
        crc = CRC8.calculate(crc_data)
        # Verify it's a valid byte
        assert 0 <= crc <= 255
        # This should be deterministic
        assert crc == CRC8.calculate(crc_data)

    def test_test_all_frame_crc(self):
        """Test CRC calculation for TEST_ALL command frame."""
        # TEST_ALL frame: LEN=0, CMD=0x10
        crc_data = bytes([0x00, Command.TEST_ALL])
        crc = CRC8.calculate(crc_data)
        assert 0 <= crc <= 255

    def test_test_single_frame_crc(self):
        """Test CRC calculation for TEST_SINGLE command frame."""
        # TEST_SINGLE frame: LEN=1, CMD=0x11, PAYLOAD=[0x01]
        crc_data = bytes([0x01, Command.TEST_SINGLE, 0x01])
        crc = CRC8.calculate(crc_data)
        assert 0 <= crc <= 255

    def test_verify_good_crc(self):
        """Verify CRC with correct value."""
        data = bytes([0x00, 0x01])
        crc = CRC8.calculate(data)
        assert CRC8.verify(data, crc) is True

    def test_verify_bad_crc(self):
        """Verify CRC detection of corruption."""
        data = bytes([0x00, 0x01])
        crc = CRC8.calculate(data)
        # Flip all bits
        assert CRC8.verify(data, crc ^ 0xFF) is False

    def test_verify_single_bit_error(self):
        """CRC should detect single bit errors."""
        data = bytes([0x00, 0x01])
        crc = CRC8.calculate(data)
        # Flip one bit
        assert CRC8.verify(data, crc ^ 0x01) is False
        assert CRC8.verify(data, crc ^ 0x80) is False

    @pytest.mark.parametrize("data", [
        bytes([0x00, 0x10]),  # TEST_ALL
        bytes([0x01, 0x11, 0x01]),  # TEST_SINGLE MLX90640
        bytes([0x01, 0x11, 0x02]),  # TEST_SINGLE VL53L0X
        bytes([0x00, 0x12]),  # GET_SENSOR_LIST
        bytes([0x05, 0x20, 0x01, 0x00, 0x64, 0x00, 0x0A]),  # SET_SPEC
    ])
    def test_crc_deterministic(self, data):
        """CRC calculation should be deterministic."""
        crc1 = CRC8.calculate(data)
        crc2 = CRC8.calculate(data)
        assert crc1 == crc2

    def test_crc_lookup_table_size(self):
        """Verify lookup table has 256 entries."""
        assert len(CRC8._TABLE) == 256

    def test_crc_lookup_table_values(self):
        """Verify first few lookup table values match firmware."""
        # From frame.c lines 33-34
        expected_first_row = [0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15]
        assert CRC8._TABLE[:8] == expected_first_row

    def test_crc_with_payload(self):
        """Test CRC with larger payload."""
        # Simulate a SET_SPEC command with spec data
        payload = bytes([0x01, 0x0E, 0x74, 0x00, 0xC8])  # sensor_id + spec
        crc_data = bytes([len(payload) - 1, Command.SET_SPEC]) + payload
        crc = CRC8.calculate(crc_data)
        assert 0 <= crc <= 255
        assert CRC8.verify(crc_data, crc)
