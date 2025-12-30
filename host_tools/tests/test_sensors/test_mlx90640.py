"""
MLX90640 IR thermal camera specific tests.
"""

import pytest
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol import SensorID, TestStatus, MLX90640Spec


@pytest.mark.hardware
class TestMLX90640:
    """MLX90640 IR thermal camera specific tests."""

    def test_sensor_present(self, client):
        """Verify MLX90640 is in sensor list."""
        sensors = client.get_sensor_list()
        ids = [s.sensor_id for s in sensors]
        assert SensorID.MLX90640 in ids

    def test_temperature_measurement_range(self, client):
        """Verify temperature measurement is within sensor range."""
        # Set wide tolerance
        spec = MLX90640Spec(target_temp=2500, tolerance=10000)  # 25C +/- 100C
        client.set_spec_mlx90640(spec)

        report = client.test_single(SensorID.MLX90640)
        result = report.results[0]

        if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
            mlx_result = result.result
            # Temperature should be in sensor range (-40C to 300C)
            assert -4000 <= mlx_result.max_temp <= 30000
            print(f"Measured temperature: {mlx_result.max_temp_celsius:.2f}C")

    def test_room_temperature(self, client):
        """Test with typical room temperature spec."""
        # Room temperature: 20-25C with tolerance
        spec = MLX90640Spec(target_temp=2200, tolerance=1500)  # 22C +/- 15C
        client.set_spec_mlx90640(spec)

        report = client.test_single(SensorID.MLX90640)
        result = report.results[0]

        if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
            # Should measure something close to room temp
            temp_c = result.result.max_temp_celsius
            print(f"Room temperature measurement: {temp_c:.2f}C")
            # Reasonable room temp range
            assert 10 <= temp_c <= 40

    def test_tolerance_pass(self, client):
        """Test should pass when within wide tolerance."""
        # Very wide tolerance to ensure pass
        spec = MLX90640Spec(target_temp=2500, tolerance=20000)  # 25C +/- 200C
        client.set_spec_mlx90640(spec)

        report = client.test_single(SensorID.MLX90640)
        result = report.results[0]

        # Should pass unless sensor error
        if result.status not in (TestStatus.FAIL_NO_ACK, TestStatus.FAIL_INIT, TestStatus.FAIL_TIMEOUT):
            assert result.status == TestStatus.PASS

    def test_tolerance_fail(self, client):
        """Test should fail with impossible spec."""
        # Impossible spec: 300C with 0.01C tolerance
        spec = MLX90640Spec(target_temp=30000, tolerance=1)
        client.set_spec_mlx90640(spec)

        report = client.test_single(SensorID.MLX90640)
        result = report.results[0]

        # Should fail unless sensor error
        if result.status not in (TestStatus.FAIL_NO_ACK, TestStatus.FAIL_INIT, TestStatus.FAIL_TIMEOUT):
            assert result.status == TestStatus.FAIL_INVALID

    def test_result_diff_calculation(self, client):
        """Verify difference is calculated correctly."""
        spec = MLX90640Spec(target_temp=2500, tolerance=5000)
        client.set_spec_mlx90640(spec)

        report = client.test_single(SensorID.MLX90640)
        result = report.results[0]

        if result.status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
            mlx_result = result.result
            expected_diff = abs(mlx_result.max_temp - mlx_result.target)
            assert mlx_result.diff == expected_diff

    def test_negative_temperature_spec(self, client):
        """Test with negative temperature specification."""
        spec = MLX90640Spec(target_temp=-2000, tolerance=5000)  # -20C +/- 50C
        client.set_spec_mlx90640(spec)

        # Just verify spec is accepted
        retrieved = client.get_spec_mlx90640()
        assert retrieved.target_temp == -2000

    def test_high_temperature_spec(self, client):
        """Test with high temperature specification."""
        spec = MLX90640Spec(target_temp=15000, tolerance=5000)  # 150C +/- 50C
        client.set_spec_mlx90640(spec)

        # Verify spec is accepted
        retrieved = client.get_spec_mlx90640()
        assert retrieved.target_temp == 15000

    @pytest.mark.slow
    def test_measurement_repeatability(self, client):
        """Multiple measurements should be reasonably consistent."""
        spec = MLX90640Spec(target_temp=2500, tolerance=10000)
        client.set_spec_mlx90640(spec)

        measurements = []
        for _ in range(10):
            report = client.test_single(SensorID.MLX90640)
            if report.results[0].status in (TestStatus.PASS, TestStatus.FAIL_INVALID):
                measurements.append(report.results[0].result.max_temp)

        if len(measurements) >= 5:
            avg = sum(measurements) / len(measurements)
            for m in measurements:
                # Within 5C of average
                assert abs(m - avg) < 500, f"Measurement {m/100}C too far from avg {avg/100}C"

            print(f"MLX90640 repeatability: avg={avg/100:.2f}C, "
                  f"range={min(measurements)/100:.2f}-{max(measurements)/100:.2f}C")

    def test_spec_celsius_properties(self, client):
        """Test Celsius conversion properties."""
        spec = MLX90640Spec(target_temp=3700, tolerance=200)

        assert spec.target_celsius == 37.0
        assert spec.tolerance_celsius == 2.0
