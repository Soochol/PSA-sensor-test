"""
GET_SENSOR_LIST tests.

Tests sensor enumeration functionality.
"""

import pytest
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from psa_protocol import SensorID


@pytest.mark.hardware
class TestSensorList:
    """Test GET_SENSOR_LIST command."""

    def test_get_sensor_list(self, client):
        """Should return list of registered sensors."""
        sensors = client.get_sensor_list()
        assert len(sensors) >= 2

    def test_sensor_list_contains_mlx90640(self, client):
        """MLX90640 should be in sensor list."""
        sensors = client.get_sensor_list()
        ids = [s.sensor_id for s in sensors]
        assert SensorID.MLX90640 in ids

    def test_sensor_list_contains_vl53l0x(self, client):
        """VL53L0X should be in sensor list."""
        sensors = client.get_sensor_list()
        ids = [s.sensor_id for s in sensors]
        assert SensorID.VL53L0X in ids

    def test_sensor_names_not_empty(self, client):
        """Sensor names should be non-empty."""
        sensors = client.get_sensor_list()
        for sensor in sensors:
            assert len(sensor.name) > 0

    def test_sensor_names_printable(self, client):
        """Sensor names should contain printable characters."""
        sensors = client.get_sensor_list()
        for sensor in sensors:
            assert sensor.name.isprintable()

    def test_sensor_list_consistent(self, client):
        """Multiple calls should return consistent results."""
        sensors1 = client.get_sensor_list()
        sensors2 = client.get_sensor_list()

        assert len(sensors1) == len(sensors2)

        for s1, s2 in zip(sensors1, sensors2):
            assert s1.sensor_id == s2.sensor_id
            assert s1.name == s2.name

    def test_sensor_ids_unique(self, client):
        """Sensor IDs should be unique."""
        sensors = client.get_sensor_list()
        ids = [s.sensor_id for s in sensors]
        assert len(ids) == len(set(ids))

    def test_expected_sensor_names(self, client):
        """Verify expected sensor names."""
        sensors = client.get_sensor_list()
        sensor_dict = {s.sensor_id: s.name for s in sensors}

        # MLX90640 should be named appropriately
        if SensorID.MLX90640 in sensor_dict:
            assert "MLX" in sensor_dict[SensorID.MLX90640].upper()

        # VL53L0X should be named appropriately
        if SensorID.VL53L0X in sensor_dict:
            assert "VL53" in sensor_dict[SensorID.VL53L0X].upper()
