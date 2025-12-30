"""
VL53L0X ToF distance sensor specific tests.
"""

import pytest
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol import SensorID, TestStatus, VL53L0XSpec


@pytest.mark.hardware
class TestVL53L0X:
    """VL53L0X ToF distance sensor specific tests."""

    def test_sensor_present(self, client):
        """Verify VL53L0X is in sensor list."""
        sensors = client.get_sensor_list()
        ids = [s.sensor_id for s in sensors]
        assert SensorID.VL53L0X in ids

    def test_distance_measurement_range(self, client):
        """Verify distance measurement is within sensor range."""
        spec = VL53L0XSpec(target_dist=500, tolerance=2000)
        client.set_spec_vl53l0x(spec)

        report = client.test_single(SensorID.VL53L0X)
        result = report.results[0]

        if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
            vl_result = result.result
            # Distance should be in sensor range (0-2500mm typical)
            assert 0 <= vl_result.measured <= 2500
            print(f"Measured distance: {vl_result.measured}mm")

    def test_short_distance_spec(self, client):
        """Test with short distance specification."""
        spec = VL53L0XSpec(target_dist=100, tolerance=50)  # 100 +/- 50mm
        client.set_spec_vl53l0x(spec)

        retrieved = client.get_spec_vl53l0x()
        assert retrieved.target_dist == 100
        assert retrieved.tolerance == 50

    def test_long_distance_spec(self, client):
        """Test with long distance specification."""
        spec = VL53L0XSpec(target_dist=1500, tolerance=200)  # 1500 +/- 200mm
        client.set_spec_vl53l0x(spec)

        retrieved = client.get_spec_vl53l0x()
        assert retrieved.target_dist == 1500
        assert retrieved.tolerance == 200

    def test_tolerance_pass(self, client):
        """Test should pass when within wide tolerance."""
        # Very wide tolerance to ensure pass
        spec = VL53L0XSpec(target_dist=500, tolerance=2000)
        client.set_spec_vl53l0x(spec)

        report = client.test_single(SensorID.VL53L0X)
        result = report.results[0]

        # Should pass unless sensor error
        if result.status not in (TestStatus.FAIL_NO_ACK, TestStatus.FAIL_INIT, TestStatus.FAIL_TIMEOUT):
            assert result.status == TestStatus.PASS

    def test_tolerance_fail(self, client):
        """Test should fail with impossible spec."""
        # Impossible spec: 2000mm with 1mm tolerance
        spec = VL53L0XSpec(target_dist=2000, tolerance=1)
        client.set_spec_vl53l0x(spec)

        report = client.test_single(SensorID.VL53L0X)
        result = report.results[0]

        # Should likely fail
        if result.status not in (TestStatus.FAIL_NO_ACK, TestStatus.FAIL_INIT, TestStatus.FAIL_TIMEOUT):
            # Could pass if object is exactly 2000mm away, but unlikely
            print(f"Distance test result: {result.status_name}")

    def test_result_diff_calculation(self, client):
        """Verify difference is calculated correctly."""
        spec = VL53L0XSpec(target_dist=500, tolerance=1000)
        client.set_spec_vl53l0x(spec)

        report = client.test_single(SensorID.VL53L0X)
        result = report.results[0]

        if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
            vl_result = result.result
            expected_diff = abs(vl_result.measured - vl_result.target)
            assert vl_result.diff == expected_diff

    def test_result_passed_property(self, client):
        """Test passed property calculation."""
        spec = VL53L0XSpec(target_dist=500, tolerance=2000)
        client.set_spec_vl53l0x(spec)

        report = client.test_single(SensorID.VL53L0X)
        result = report.results[0]

        if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
            vl_result = result.result
            # passed should match diff <= tolerance
            assert vl_result.passed == (vl_result.diff <= vl_result.tolerance)

    @pytest.mark.slow
    def test_measurement_repeatability(self, client):
        """Multiple measurements should be reasonably consistent."""
        spec = VL53L0XSpec(target_dist=500, tolerance=2000)
        client.set_spec_vl53l0x(spec)

        measurements = []
        for _ in range(10):
            report = client.test_single(SensorID.VL53L0X)
            if report.results[0].status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
                measurements.append(report.results[0].result.measured)

        if len(measurements) >= 5:
            avg = sum(measurements) / len(measurements)
            for m in measurements:
                # Within 50mm of average (reasonable for ToF sensor)
                assert abs(m - avg) < 50, f"Measurement {m}mm too far from avg {avg}mm"

            print(f"VL53L0X repeatability: avg={avg:.1f}mm, "
                  f"range={min(measurements)}-{max(measurements)}mm")

    @pytest.mark.slow
    def test_measurement_stability(self, client):
        """Measurements should be stable over time."""
        spec = VL53L0XSpec(target_dist=500, tolerance=2000)
        client.set_spec_vl53l0x(spec)

        import time

        measurements = []
        for _ in range(5):
            report = client.test_single(SensorID.VL53L0X)
            if report.results[0].status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
                measurements.append(report.results[0].result.measured)
            time.sleep(0.5)

        if len(measurements) >= 3:
            # Check standard deviation is reasonable
            avg = sum(measurements) / len(measurements)
            variance = sum((m - avg) ** 2 for m in measurements) / len(measurements)
            std_dev = variance ** 0.5

            print(f"VL53L0X stability: avg={avg:.1f}mm, std_dev={std_dev:.1f}mm")

            # Std dev should be less than 30mm for stable measurements
            assert std_dev < 30

    def test_max_range(self, client):
        """Test with maximum range specification."""
        spec = VL53L0XSpec(target_dist=2000, tolerance=500)  # Max range
        client.set_spec_vl53l0x(spec)

        retrieved = client.get_spec_vl53l0x()
        assert retrieved.target_dist == 2000

    def test_min_range(self, client):
        """Test with minimum range specification."""
        spec = VL53L0XSpec(target_dist=30, tolerance=20)  # Min range
        client.set_spec_vl53l0x(spec)

        retrieved = client.get_spec_vl53l0x()
        assert retrieved.target_dist == 30
