"""
PSA Sensor Test Sequence Package

Provides automated test sequence for VL53L0X and MLX90640 sensors.
"""

# Configure loguru to disable default stderr handler
# This prevents duplicate logging when running under station-service
import sys
from loguru import logger

logger.remove()  # Remove default stderr handler
logger.add(
    sys.stdout,
    level="DEBUG",
    format="{time:HH:mm:ss} | {name}:{function}:{line} - {message}"
)  # Custom format without level (UI badge shows level separately)

from .sequence import PSASensorTestSequence

__all__ = ["PSASensorTestSequence"]
