"""
Frame parsing and building unit tests.
"""

import pytest
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol.frame import Frame, FrameBuilder, FrameParser, ParseResult
from psa_protocol.constants import STX, ETX, MAX_PAYLOAD, Command, Response, SensorID
from psa_protocol.crc import CRC8


@pytest.mark.unit
class TestFrame:
    """Test Frame dataclass."""

    def test_frame_creation(self):
        """Test creating a frame."""
        frame = Frame(cmd=Command.PING)
        assert frame.cmd == Command.PING
        assert frame.payload == b''
        assert frame.payload_len == 0

    def test_frame_with_payload(self):
        """Test creating a frame with payload."""
        frame = Frame(cmd=Command.TEST_SINGLE, payload=bytes([0x01]))
        assert frame.cmd == Command.TEST_SINGLE
        assert frame.payload == bytes([0x01])
        assert frame.payload_len == 1

    def test_frame_payload_list(self):
        """Test creating a frame with list payload."""
        frame = Frame(cmd=Command.SET_SPEC, payload=[0x01, 0x02, 0x03])
        assert frame.payload == bytes([0x01, 0x02, 0x03])

    def test_frame_max_payload(self):
        """Test frame with maximum payload."""
        payload = bytes([0x00] * MAX_PAYLOAD)
        frame = Frame(cmd=Command.SET_SPEC, payload=payload)
        assert frame.payload_len == MAX_PAYLOAD

    def test_frame_payload_too_large(self):
        """Test frame rejects payload larger than max."""
        payload = bytes([0x00] * (MAX_PAYLOAD + 1))
        with pytest.raises(ValueError, match="exceeds maximum"):
            Frame(cmd=Command.SET_SPEC, payload=payload)


@pytest.mark.unit
class TestFrameBuilder:
    """Test frame construction."""

    def test_build_ping(self):
        """Test PING frame construction."""
        frame_bytes = FrameBuilder.build_ping()

        assert frame_bytes[0] == STX
        assert frame_bytes[1] == 0  # LEN
        assert frame_bytes[2] == Command.PING
        assert frame_bytes[-1] == ETX
        assert len(frame_bytes) == 5  # STX + LEN + CMD + CRC + ETX

    def test_build_test_all(self):
        """Test TEST_ALL frame construction."""
        frame_bytes = FrameBuilder.build_test_all()

        assert frame_bytes[0] == STX
        assert frame_bytes[1] == 0  # LEN
        assert frame_bytes[2] == Command.TEST_ALL
        assert frame_bytes[-1] == ETX

    def test_build_test_single(self):
        """Test TEST_SINGLE frame construction."""
        frame_bytes = FrameBuilder.build_test_single(SensorID.MLX90640)

        assert frame_bytes[0] == STX
        assert frame_bytes[1] == 1  # LEN (1 byte payload)
        assert frame_bytes[2] == Command.TEST_SINGLE
        assert frame_bytes[3] == SensorID.MLX90640  # Sensor ID
        assert frame_bytes[-1] == ETX
        assert len(frame_bytes) == 6  # STX + LEN + CMD + PAYLOAD + CRC + ETX

    def test_build_get_sensor_list(self):
        """Test GET_SENSOR_LIST frame construction."""
        frame_bytes = FrameBuilder.build_get_sensor_list()

        assert frame_bytes[0] == STX
        assert frame_bytes[1] == 0  # LEN
        assert frame_bytes[2] == Command.GET_SENSOR_LIST
        assert frame_bytes[-1] == ETX

    def test_build_set_spec(self):
        """Test SET_SPEC frame construction."""
        spec_data = bytes([0x00, 0x64, 0x00, 0x0A])  # target=100, tolerance=10
        frame_bytes = FrameBuilder.build_set_spec(SensorID.MLX90640, spec_data)

        assert frame_bytes[0] == STX
        assert frame_bytes[1] == 5  # LEN (sensor_id + 4 bytes spec)
        assert frame_bytes[2] == Command.SET_SPEC
        assert frame_bytes[3] == SensorID.MLX90640  # Sensor ID
        assert frame_bytes[4:8] == spec_data
        assert frame_bytes[-1] == ETX

    def test_build_get_spec(self):
        """Test GET_SPEC frame construction."""
        frame_bytes = FrameBuilder.build_get_spec(SensorID.VL53L0X)

        assert frame_bytes[0] == STX
        assert frame_bytes[1] == 1  # LEN
        assert frame_bytes[2] == Command.GET_SPEC
        assert frame_bytes[3] == SensorID.VL53L0X
        assert frame_bytes[-1] == ETX

    def test_build_crc_correct(self):
        """Test that built frames have correct CRC."""
        frame_bytes = FrameBuilder.build_ping()

        # Extract CRC data: LEN + CMD + PAYLOAD
        payload_len = frame_bytes[1]
        crc_data = frame_bytes[1:3 + payload_len]
        expected_crc = CRC8.calculate(crc_data)

        # CRC is second-to-last byte
        actual_crc = frame_bytes[-2]
        assert actual_crc == expected_crc

    def test_build_from_frame(self):
        """Test building from Frame object."""
        frame = Frame(cmd=Command.TEST_SINGLE, payload=bytes([SensorID.VL53L0X]))
        frame_bytes = FrameBuilder.build(frame)

        assert frame_bytes[0] == STX
        assert frame_bytes[2] == Command.TEST_SINGLE
        assert frame_bytes[-1] == ETX


@pytest.mark.unit
class TestFrameParser:
    """Test frame parsing."""

    def test_parse_ping_response(self):
        """Test parsing PONG response."""
        # Build a valid PONG response: [STX][LEN=3][CMD=0x01][1][0][0][CRC][ETX]
        payload = bytes([1, 0, 0])  # Version 1.0.0
        crc_data = bytes([len(payload), Response.PONG]) + payload
        crc = CRC8.calculate(crc_data)
        frame_bytes = bytes([STX, len(payload), Response.PONG]) + payload + bytes([crc, ETX])

        parser = FrameParser()
        parser.feed(frame_bytes)
        result, frame, consumed = parser.parse()

        assert result == ParseResult.OK
        assert frame.cmd == Response.PONG
        assert frame.payload == payload
        assert consumed == len(frame_bytes)

    def test_parse_empty_payload(self):
        """Test parsing frame with empty payload."""
        crc_data = bytes([0, Command.PING])
        crc = CRC8.calculate(crc_data)
        frame_bytes = bytes([STX, 0, Command.PING, crc, ETX])

        parser = FrameParser()
        parser.feed(frame_bytes)
        result, frame, consumed = parser.parse()

        assert result == ParseResult.OK
        assert frame.cmd == Command.PING
        assert frame.payload == b''

    def test_parse_incomplete_frame(self):
        """Test handling of incomplete frame."""
        parser = FrameParser()
        parser.feed(bytes([STX, 0x03]))  # Only STX and LEN

        result, frame, consumed = parser.parse()

        assert result == ParseResult.INCOMPLETE
        assert frame is None

    def test_parse_incomplete_no_etx(self):
        """Test handling of frame without ETX."""
        parser = FrameParser()
        # Missing ETX
        parser.feed(bytes([STX, 0x00, Command.PING, 0x07]))

        result, frame, consumed = parser.parse()

        assert result == ParseResult.INCOMPLETE
        assert frame is None

    def test_parse_crc_error(self):
        """Test CRC error detection."""
        # Frame with intentionally wrong CRC
        parser = FrameParser()
        frame_bytes = bytes([STX, 0x00, Command.PING, 0xFF, ETX])  # Wrong CRC
        parser.feed(frame_bytes)

        result, frame, consumed = parser.parse()

        assert result == ParseResult.CRC_ERROR
        assert frame is None

    def test_parse_format_error_bad_etx(self):
        """Test format error when ETX is wrong."""
        parser = FrameParser()
        crc_data = bytes([0, Command.PING])
        crc = CRC8.calculate(crc_data)
        # Wrong ETX (0xFF instead of 0x03)
        frame_bytes = bytes([STX, 0, Command.PING, crc, 0xFF])
        parser.feed(frame_bytes)

        result, frame, consumed = parser.parse()

        assert result == ParseResult.FORMAT_ERROR
        assert frame is None

    def test_parse_format_error_large_len(self):
        """Test format error when LEN exceeds maximum."""
        parser = FrameParser()
        # LEN = 100 (exceeds MAX_PAYLOAD)
        frame_bytes = bytes([STX, 100])
        parser.feed(frame_bytes)

        result, frame, consumed = parser.parse()

        assert result == ParseResult.FORMAT_ERROR
        assert frame is None

    def test_parse_multiple_frames(self):
        """Test parsing multiple consecutive frames."""
        parser = FrameParser()

        # Build two valid frames
        frames_data = bytearray()
        for _ in range(2):
            crc_data = bytes([0, Command.PING])
            crc = CRC8.calculate(crc_data)
            frames_data.extend([STX, 0, Command.PING, crc, ETX])

        parser.feed(bytes(frames_data))

        # Parse first frame
        result1, frame1, _ = parser.parse()
        assert result1 == ParseResult.OK
        assert frame1.cmd == Command.PING

        # Parse second frame
        result2, frame2, _ = parser.parse()
        assert result2 == ParseResult.OK
        assert frame2.cmd == Command.PING

    def test_parse_garbage_before_frame(self):
        """Test skipping garbage bytes before STX."""
        parser = FrameParser()

        # Garbage + valid frame
        garbage = bytes([0xFF, 0xAA, 0x55])
        crc_data = bytes([0, Command.PING])
        crc = CRC8.calculate(crc_data)
        valid_frame = bytes([STX, 0, Command.PING, crc, ETX])

        parser.feed(garbage + valid_frame)
        result, frame, consumed = parser.parse()

        assert result == ParseResult.OK
        assert frame.cmd == Command.PING

    def test_parse_garbage_between_frames(self):
        """Test handling garbage between frames."""
        parser = FrameParser()

        # Build valid frame
        crc_data = bytes([0, Command.PING])
        crc = CRC8.calculate(crc_data)
        valid_frame = bytes([STX, 0, Command.PING, crc, ETX])

        # First frame + garbage + second frame
        parser.feed(valid_frame + bytes([0xAA, 0xBB]) + valid_frame)

        # Parse first frame
        result1, frame1, _ = parser.parse()
        assert result1 == ParseResult.OK

        # Parse second frame (should skip garbage)
        result2, frame2, _ = parser.parse()
        assert result2 == ParseResult.OK

    def test_parser_clear(self):
        """Test clearing parser buffer."""
        parser = FrameParser()
        parser.feed(bytes([STX, 0x00, 0x01]))  # Partial frame
        assert parser.buffer_size > 0

        parser.clear()
        assert parser.buffer_size == 0

    def test_parse_incremental_feed(self):
        """Test parsing with incremental data feeding."""
        parser = FrameParser()

        crc_data = bytes([0, Command.PING])
        crc = CRC8.calculate(crc_data)
        full_frame = bytes([STX, 0, Command.PING, crc, ETX])

        # Feed one byte at a time
        for i, byte in enumerate(full_frame[:-1]):
            parser.feed(bytes([byte]))
            result, frame, _ = parser.parse()
            assert result == ParseResult.INCOMPLETE

        # Feed last byte
        parser.feed(bytes([full_frame[-1]]))
        result, frame, _ = parser.parse()
        assert result == ParseResult.OK
        assert frame.cmd == Command.PING

    def test_parse_nak_response(self):
        """Test parsing NAK response."""
        error_code = 0x01  # UNKNOWN_CMD
        payload = bytes([error_code])
        crc_data = bytes([len(payload), Response.NAK]) + payload
        crc = CRC8.calculate(crc_data)
        frame_bytes = bytes([STX, len(payload), Response.NAK]) + payload + bytes([crc, ETX])

        parser = FrameParser()
        parser.feed(frame_bytes)
        result, frame, _ = parser.parse()

        assert result == ParseResult.OK
        assert frame.cmd == Response.NAK
        assert frame.payload[0] == error_code

    def test_parse_test_result_response(self):
        """Test parsing TEST_RESULT response."""
        # Minimal test result: 1 sensor, pass, with result data
        payload = bytes([
            1,    # sensor_count
            1,    # pass_count
            0,    # fail_count
            0, 0, 0, 100,  # timestamp (big-endian)
            0x01,  # sensor_id (MLX90640)
            0x00,  # status (PASS)
            # result data (8 bytes)
            0x0E, 0x74,  # max_temp (37.00 C)
            0x0E, 0x74,  # target
            0x00, 0xC8,  # tolerance (2.00 C)
            0x00, 0x00,  # diff (0)
        ])
        crc_data = bytes([len(payload), Response.TEST_RESULT]) + payload
        crc = CRC8.calculate(crc_data)
        frame_bytes = bytes([STX, len(payload), Response.TEST_RESULT]) + payload + bytes([crc, ETX])

        parser = FrameParser()
        parser.feed(frame_bytes)
        result, frame, _ = parser.parse()

        assert result == ParseResult.OK
        assert frame.cmd == Response.TEST_RESULT
        assert len(frame.payload) == len(payload)
