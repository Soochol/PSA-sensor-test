"""
TEST_SINGLE tests.

Tests single sensor testing functionality.
"""

import pytest
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol import (
    SensorID, TestStatus,
    MLX90640Spec, VL53L0XSpec,
    NAKError
)


@pytest.mark.hardware
class TestSingleSensor:
    """Test TEST_SINGLE command."""

    @pytest.fixture(autouse=True)
    def setup_specs(self, client):
        """Setup specifications before each test."""
        # Set wide tolerances for testing
        client.set_spec_mlx90640(MLX90640Spec(target_temp=2500, tolerance=5000))
        client.set_spec_vl53l0x(VL53L0XSpec(target_dist=500, tolerance=2000))

    def test_single_mlx90640(self, client):
        """Test single MLX90640 sensor."""
        report = client.test_single(SensorID.MLX90640)

        assert report.sensor_count == 1
        assert len(report.results) == 1
        assert report.results[0].sensor_id == SensorID.MLX90640

        # Status should be defined
        valid_statuses = [
            TestStatus.PASS,
            TestStatus.FAIL_INVALID,
            TestStatus.FAIL_NO_ACK,
            TestStatus.FAIL_TIMEOUT,
            TestStatus.FAIL_INIT
        ]
        assert report.results[0].status in valid_statuses

    def test_single_vl53l0x(self, client):
        """Test single VL53L0X sensor."""
        report = client.test_single(SensorID.VL53L0X)

        assert report.sensor_count == 1
        assert len(report.results) == 1
        assert report.results[0].sensor_id == SensorID.VL53L0X

    def test_single_sensor_result_data(self, client):
        """Verify result data is populated on success."""
        report = client.test_single(SensorID.MLX90640)
        result = report.results[0]

        if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
            # Result should have data
            assert result.result is not None
            # Temperature should be in reasonable range
            assert -4000 <= result.result.max_temp <= 30000

    def test_single_pass_count(self, client):
        """Verify pass count is correct."""
        report = client.test_single(SensorID.MLX90640)

        if report.results[0].status == TestStatus.PASS:
            assert report.pass_count == 1
            assert report.fail_count == 0
        else:
            assert report.pass_count == 0

    def test_single_fail_count(self, client):
        """Set impossible spec and verify failure."""
        # Set spec that's impossible to meet
        client.set_spec_mlx90640(MLX90640Spec(target_temp=30000, tolerance=1))  # 300C +/- 0.01C

        report = client.test_single(SensorID.MLX90640)
        result = report.results[0]

        # Should fail unless sensor error
        if result.status not in (TestStatus.FAIL_NO_ACK, TestStatus.FAIL_INIT, TestStatus.FAIL_TIMEOUT):
            assert result.status == TestStatus.FAIL_INVALID
            assert report.fail_count == 1

    def test_single_timestamp(self, client):
        """Verify timestamp is present."""
        report = client.test_single(SensorID.VL53L0X)

        # Timestamp should be non-zero if device has been running
        assert report.timestamp >= 0

    def test_single_invalid_sensor(self, client):
        """TEST_SINGLE with invalid sensor ID should return NAK."""
        with pytest.raises(NAKError):
            client.test_single(0xFF)

    @pytest.mark.slow
    def test_single_repeated_mlx90640(self, client):
        """Multiple sequential tests should be consistent."""
        client.set_spec_mlx90640(MLX90640Spec(target_temp=2500, tolerance=5000))

        results = []
        for _ in range(5):
            report = client.test_single(SensorID.MLX90640)
            result = report.results[0]
            if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
                results.append(result.result.max_temp)

        if len(results) >= 3:
            # Check measurements are reasonably consistent (within 10C)
            avg = sum(results) / len(results)
            for r in results:
                assert abs(r - avg) < 1000  # Within 10C

    @pytest.mark.slow
    def test_single_repeated_vl53l0x(self, client):
        """Multiple VL53L0X tests should be consistent."""
        client.set_spec_vl53l0x(VL53L0XSpec(target_dist=500, tolerance=2000))

        results = []
        for _ in range(5):
            report = client.test_single(SensorID.VL53L0X)
            result = report.results[0]
            if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
                results.append(result.result.measured)

        if len(results) >= 3:
            # Check measurements are reasonably consistent (within 100mm)
            avg = sum(results) / len(results)
            for r in results:
                assert abs(r - avg) < 100
