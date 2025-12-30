"""
SET_SPEC and GET_SPEC tests.

Tests sensor specification management.
"""

import pytest
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol import (
    SensorID, ErrorCode,
    MLX90640Spec, VL53L0XSpec,
    NAKError
)
from psa_protocol.frame import FrameBuilder


@pytest.mark.hardware
class TestSpecManagement:
    """Test SET_SPEC and GET_SPEC commands."""

    def test_set_get_mlx90640_spec(self, client):
        """Set and retrieve MLX90640 specification."""
        spec = MLX90640Spec(target_temp=3500, tolerance=500)  # 35.00 +/- 5.00 C

        result = client.set_spec_mlx90640(spec)
        assert result is True

        retrieved = client.get_spec_mlx90640()
        assert retrieved.target_temp == spec.target_temp
        assert retrieved.tolerance == spec.tolerance

    def test_set_get_vl53l0x_spec(self, client):
        """Set and retrieve VL53L0X specification."""
        spec = VL53L0XSpec(target_dist=500, tolerance=50)  # 500 +/- 50 mm

        result = client.set_spec_vl53l0x(spec)
        assert result is True

        retrieved = client.get_spec_vl53l0x()
        assert retrieved.target_dist == spec.target_dist
        assert retrieved.tolerance == spec.tolerance

    def test_mlx90640_spec_negative_temp(self, client):
        """MLX90640 spec should handle negative temperatures."""
        spec = MLX90640Spec(target_temp=-2000, tolerance=500)  # -20.00 +/- 5.00 C

        result = client.set_spec_mlx90640(spec)
        assert result is True

        retrieved = client.get_spec_mlx90640()
        assert retrieved.target_temp == -2000
        assert retrieved.tolerance == 500

    def test_mlx90640_spec_high_temp(self, client):
        """MLX90640 spec should handle high temperatures."""
        spec = MLX90640Spec(target_temp=25000, tolerance=1000)  # 250.00 +/- 10.00 C

        result = client.set_spec_mlx90640(spec)
        assert result is True

        retrieved = client.get_spec_mlx90640()
        assert retrieved.target_temp == 25000

    def test_vl53l0x_spec_short_distance(self, client):
        """VL53L0X spec should handle short distances."""
        spec = VL53L0XSpec(target_dist=50, tolerance=10)  # 50 +/- 10 mm

        result = client.set_spec_vl53l0x(spec)
        assert result is True

        retrieved = client.get_spec_vl53l0x()
        assert retrieved.target_dist == 50

    def test_vl53l0x_spec_long_distance(self, client):
        """VL53L0X spec should handle long distances."""
        spec = VL53L0XSpec(target_dist=1500, tolerance=100)  # 1500 +/- 100 mm

        result = client.set_spec_vl53l0x(spec)
        assert result is True

        retrieved = client.get_spec_vl53l0x()
        assert retrieved.target_dist == 1500

    def test_spec_persistence(self, client):
        """Spec should persist across multiple gets."""
        spec = MLX90640Spec(target_temp=4200, tolerance=200)
        client.set_spec_mlx90640(spec)

        # Get multiple times
        for _ in range(5):
            retrieved = client.get_spec_mlx90640()
            assert retrieved.target_temp == 4200
            assert retrieved.tolerance == 200

    def test_spec_update(self, client):
        """Spec should be updateable."""
        # Set initial spec
        spec1 = MLX90640Spec(target_temp=2500, tolerance=100)
        client.set_spec_mlx90640(spec1)

        # Update to new spec
        spec2 = MLX90640Spec(target_temp=3700, tolerance=200)
        client.set_spec_mlx90640(spec2)

        # Verify new spec
        retrieved = client.get_spec_mlx90640()
        assert retrieved.target_temp == 3700
        assert retrieved.tolerance == 200

    def test_invalid_sensor_id_set_spec(self, client, transport):
        """SET_SPEC with invalid sensor ID should return NAK."""
        # Build frame with invalid sensor ID 0xFF
        spec_data = bytes([0x00, 0x64, 0x00, 0x0A])
        frame = FrameBuilder.build_set_spec(0xFF, spec_data)

        transport.flush()
        transport.send(frame)

        # Try to receive response
        import time
        from psa_protocol.frame import FrameParser, ParseResult
        from psa_protocol.constants import Response

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

    def test_invalid_sensor_id_get_spec(self, client, transport):
        """GET_SPEC with invalid sensor ID should return NAK."""
        frame = FrameBuilder.build_get_spec(0xFF)

        transport.flush()
        transport.send(frame)

        import time
        from psa_protocol.frame import FrameParser, ParseResult
        from psa_protocol.constants import Response

        parser = FrameParser()
        start = time.time()
        while time.time() - start < 2.0:
            data = transport.receive(timeout=0.1)
            if data:
                parser.feed(data)
                result, resp_frame, _ = parser.parse()
                if result == ParseResult.OK:
                    assert resp_frame.cmd == Response.NAK
                    return

        pytest.fail("No NAK response received")
