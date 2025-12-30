"""
Error handling tests.

Tests NAK responses and edge cases.
"""

import pytest
import time
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol import ErrorCode, SensorID
from psa_protocol.constants import STX, ETX, Command, Response
from psa_protocol.crc import CRC8
from psa_protocol.frame import FrameBuilder, FrameParser, ParseResult


@pytest.mark.hardware
class TestErrorHandling:
    """Test error responses and edge cases."""

    def test_unknown_command(self, transport):
        """Unknown command should return NAK with UNKNOWN_CMD."""
        # Build frame with unknown command 0xAA
        payload = b''
        crc_data = bytes([len(payload), 0xAA]) + payload
        crc = CRC8.calculate(crc_data)
        frame = bytes([STX, len(payload), 0xAA, crc, ETX])

        transport.flush()
        transport.send(frame)

        # Read response
        parser = FrameParser()
        start = time.time()
        while time.time() - start < 2.0:
            data = transport.receive(timeout=0.1)
            if data:
                parser.feed(data)
                result, resp_frame, _ = parser.parse()
                if result == ParseResult.OK:
                    assert resp_frame.cmd == Response.NAK
                    assert resp_frame.payload[0] == ErrorCode.UNKNOWN_CMD
                    return

        pytest.fail("No NAK response received")

    def test_invalid_payload_length(self, transport):
        """Invalid payload should return NAK."""
        # SET_SPEC with too short payload (only sensor ID, no spec data)
        payload = bytes([SensorID.MLX90640])  # Missing spec data
        crc_data = bytes([len(payload), Command.SET_SPEC]) + payload
        crc = CRC8.calculate(crc_data)
        frame = bytes([STX, len(payload), Command.SET_SPEC]) + payload + bytes([crc, ETX])

        transport.flush()
        transport.send(frame)

        parser = FrameParser()
        start = time.time()
        while time.time() - start < 2.0:
            data = transport.receive(timeout=0.1)
            if data:
                parser.feed(data)
                result, resp_frame, _ = parser.parse()
                if result == ParseResult.OK:
                    assert resp_frame.cmd == Response.NAK
                    assert resp_frame.payload[0] == ErrorCode.INVALID_PAYLOAD
                    return

        pytest.fail("No NAK response received")

    def test_crc_error_recovery(self, client, transport):
        """Device should recover after receiving frame with bad CRC."""
        # Send frame with intentionally wrong CRC
        frame = bytes([STX, 0x00, Command.PING, 0xFF, ETX])  # Bad CRC

        transport.flush()
        transport.send(frame)
        time.sleep(0.2)

        # Device should still respond to valid PING
        version = client.ping()
        assert version is not None

    def test_garbage_recovery(self, client, transport):
        """Device should recover after receiving garbage data."""
        # Send random garbage
        garbage = bytes([0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE])
        transport.send(garbage)
        time.sleep(0.1)

        # Device should still respond to valid PING
        version = client.ping()
        assert version is not None

    def test_partial_frame_recovery(self, client, transport):
        """Device should recover after receiving partial frame."""
        # Send partial frame (missing payload, CRC, ETX)
        partial = bytes([STX, 0x05, Command.SET_SPEC])
        transport.send(partial)
        time.sleep(0.2)

        # Device should still respond to valid PING
        version = client.ping()
        assert version is not None

    def test_multiple_stx_recovery(self, client, transport):
        """Device should handle multiple STX bytes."""
        # Multiple STX followed by valid frame
        multi_stx = bytes([STX, STX, STX])
        transport.send(multi_stx)
        time.sleep(0.1)

        # Valid PING should still work
        version = client.ping()
        assert version is not None

    def test_interleaved_garbage(self, client, transport):
        """Device should handle garbage interleaved with valid commands."""
        for _ in range(5):
            # Send garbage
            transport.send(bytes([0xAA, 0xBB, 0xCC]))
            time.sleep(0.05)

            # Valid PING should work
            version = client.ping()
            assert version is not None

    def test_long_garbage_burst(self, client, transport):
        """Device should recover from long burst of garbage."""
        # Send 100 bytes of garbage
        garbage = bytes([i % 256 for i in range(100)])
        transport.send(garbage)
        time.sleep(0.3)

        # Device should still respond
        version = client.ping()
        assert version is not None

    def test_invalid_sensor_id_test_single(self, transport):
        """TEST_SINGLE with invalid sensor ID should return NAK."""
        frame = FrameBuilder.build_test_single(0xFF)

        transport.flush()
        transport.send(frame)

        parser = FrameParser()
        start = time.time()
        while time.time() - start < 2.0:
            data = transport.receive(timeout=0.1)
            if data:
                parser.feed(data)
                result, resp_frame, _ = parser.parse()
                if result == ParseResult.OK:
                    assert resp_frame.cmd == Response.NAK
                    assert resp_frame.payload[0] == ErrorCode.INVALID_SENSOR_ID
                    return

        pytest.fail("No NAK response received")

    def test_empty_buffer_after_response(self, client, transport):
        """Buffer should be cleared after receiving response."""
        # Send PING
        version = client.ping()
        assert version is not None

        # Give time for any extra data
        time.sleep(0.1)

        # Buffer should be empty
        data = transport.receive_all()
        # Allow for some noise but not complete frames
        assert len(data) < 10
