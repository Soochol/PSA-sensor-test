"""
Connection and PING tests.

Tests basic device connectivity.
"""

import pytest
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol import PSAClient
from psa_protocol.frame import FrameBuilder


@pytest.mark.hardware
class TestConnection:
    """Test basic device connectivity."""

    def test_ping_response(self, client):
        """Device should respond to PING with PONG."""
        major, minor, patch = client.ping()
        assert major >= 0
        assert minor >= 0
        assert patch >= 0

    def test_firmware_version(self, client):
        """Verify expected firmware version."""
        major, minor, patch = client.ping()
        # Expected version 1.0.0 (from config.h)
        assert major == 1
        assert minor == 0
        assert patch == 0

    def test_multiple_pings(self, client):
        """Device should handle multiple sequential pings."""
        for i in range(10):
            version = client.ping()
            assert version is not None
            assert version == (1, 0, 0)

    def test_ping_after_invalid_data(self, client, transport):
        """Device should respond to ping after receiving garbage."""
        # Send garbage
        transport.send(bytes([0xFF, 0xAA, 0x55, 0x00]))

        import time
        time.sleep(0.1)

        # PING should still work
        version = client.ping()
        assert version is not None

    def test_ping_after_partial_frame(self, client, transport):
        """Device should recover after receiving partial frame."""
        # Send partial frame (missing ETX)
        transport.send(bytes([0x02, 0x00, 0x01, 0x07]))

        import time
        time.sleep(0.1)

        # PING should still work
        version = client.ping()
        assert version is not None

    def test_rapid_pings(self, client):
        """Device should handle rapid sequential pings."""
        import time

        start = time.time()
        count = 50

        for _ in range(count):
            version = client.ping()
            assert version is not None

        elapsed = time.time() - start
        rate = count / elapsed
        print(f"Ping rate: {rate:.1f} pings/second")

        # Should be able to do at least 10 pings per second
        assert rate > 10
