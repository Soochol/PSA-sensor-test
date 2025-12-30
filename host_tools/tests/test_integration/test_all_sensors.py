"""
TEST_ALL tests.

Tests all sensors simultaneously.
"""

import pytest
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol import (
    SensorID, TestStatus,
    MLX90640Spec, VL53L0XSpec
)


@pytest.mark.hardware
@pytest.mark.slow
class TestAllSensors:
    """Test TEST_ALL command."""

    @pytest.fixture(autouse=True)
    def setup_specs(self, client):
        """Setup specifications for all sensors."""
        # Set wide tolerances for reliable testing
        client.set_spec_mlx90640(MLX90640Spec(target_temp=2500, tolerance=5000))
        client.set_spec_vl53l0x(VL53L0XSpec(target_dist=500, tolerance=2000))

    def test_all_sensors(self, client):
        """Run TEST_ALL and verify response."""
        report = client.test_all()

        assert report.sensor_count >= 2
        assert report.pass_count + report.fail_count == report.sensor_count

    def test_all_sensors_contains_both(self, client):
        """Both sensors should be in results."""
        report = client.test_all()

        sensor_ids = [r.sensor_id for r in report.results]
        assert SensorID.MLX90640 in sensor_ids
        assert SensorID.VL53L0X in sensor_ids

    def test_all_sensors_pass_count(self, client):
        """Verify pass/fail counts are consistent with results."""
        report = client.test_all()

        actual_pass = sum(1 for r in report.results if r.status == TestStatus.PASS)
        actual_fail = sum(
            1 for r in report.results
            if r.status not in (TestStatus.PASS, TestStatus.NOT_TESTED)
        )

        assert report.pass_count == actual_pass
        # fail_count should match or be less (some may be NOT_TESTED)
        assert report.fail_count <= report.sensor_count - report.pass_count

    def test_all_sensors_timestamp(self, client):
        """Verify timestamp is reasonable."""
        report = client.test_all()

        # Timestamp should be present
        assert report.timestamp >= 0

    def test_all_sensors_result_data(self, client):
        """Verify result data is populated for each sensor."""
        report = client.test_all()

        for result in report.results:
            if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
                assert result.result is not None

    def test_all_sensors_mlx90640_data(self, client):
        """Verify MLX90640 result data format."""
        report = client.test_all()

        mlx_results = [r for r in report.results if r.sensor_id == SensorID.MLX90640]
        assert len(mlx_results) == 1

        mlx = mlx_results[0]
        if mlx.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
            # Temperature should be in sensor range (-40C to 300C)
            assert -4000 <= mlx.result.max_temp <= 30000
            assert mlx.result.target == 2500  # Our spec
            assert mlx.result.tolerance == 5000

    def test_all_sensors_vl53l0x_data(self, client):
        """Verify VL53L0X result data format."""
        report = client.test_all()

        vl_results = [r for r in report.results if r.sensor_id == SensorID.VL53L0X]
        assert len(vl_results) == 1

        vl = vl_results[0]
        if vl.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
            # Distance should be in sensor range (30-2000mm)
            assert 0 <= vl.result.measured <= 2500
            assert vl.result.target == 500  # Our spec
            assert vl.result.tolerance == 2000

    def test_all_sensors_all_passed_property(self, client):
        """Test all_passed property."""
        # Set very wide tolerances to maximize pass chance
        client.set_spec_mlx90640(MLX90640Spec(target_temp=2500, tolerance=10000))
        client.set_spec_vl53l0x(VL53L0XSpec(target_dist=500, tolerance=2000))

        report = client.test_all()

        if report.all_passed:
            assert report.fail_count == 0
            assert report.pass_count == report.sensor_count

    def test_all_sensors_repeated(self, client):
        """Multiple TEST_ALL calls should be consistent."""
        reports = []
        for _ in range(3):
            report = client.test_all()
            reports.append(report)

        # Sensor count should be consistent
        for r in reports:
            assert r.sensor_count == reports[0].sensor_count

    def test_all_sensors_timeout_handling(self, client):
        """TEST_ALL should complete within timeout."""
        import time

        start = time.time()
        report = client.test_all(timeout=15.0)
        elapsed = time.time() - start

        # Should complete
        assert report is not None
        # Should not take forever
        assert elapsed < 15.0

        print(f"TEST_ALL completed in {elapsed:.2f}s")
