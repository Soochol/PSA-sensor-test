"""
pytest configuration and shared fixtures.
"""

import pytest
import os
import logging
import sys

# Add psa_protocol to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from psa_protocol import SerialTransport, PSAClient


def pytest_addoption(parser):
    """Add command line options."""
    parser.addoption(
        "--serial-port",
        action="store",
        default=os.environ.get("PSA_SERIAL_PORT", "/dev/ttyUSB0"),
        help="Serial port for device under test"
    )
    parser.addoption(
        "--baudrate",
        action="store",
        default=115200,
        type=int,
        help="Serial baudrate"
    )
    parser.addoption(
        "--response-timeout",
        action="store",
        default=5.0,
        type=float,
        help="Response timeout in seconds"
    )
    parser.addoption(
        "--skip-hardware",
        action="store_true",
        default=False,
        help="Skip tests requiring hardware"
    )


def pytest_configure(config):
    """Configure pytest markers."""
    config.addinivalue_line(
        "markers", "hardware: marks tests as requiring hardware"
    )
    config.addinivalue_line(
        "markers", "slow: marks tests as slow-running"
    )
    config.addinivalue_line(
        "markers", "unit: marks unit tests (no hardware required)"
    )


def pytest_collection_modifyitems(config, items):
    """Skip hardware tests if --skip-hardware is set."""
    if config.getoption("--skip-hardware"):
        skip_hardware = pytest.mark.skip(reason="Hardware tests skipped (--skip-hardware)")
        for item in items:
            if "hardware" in item.keywords:
                item.add_marker(skip_hardware)


@pytest.fixture(scope="session")
def serial_port(request):
    """Get serial port from command line or environment."""
    return request.config.getoption("--serial-port")


@pytest.fixture(scope="session")
def baudrate(request):
    """Get baudrate from command line."""
    return request.config.getoption("--baudrate")


@pytest.fixture(scope="session")
def response_timeout(request):
    """Get response timeout from command line."""
    return request.config.getoption("--response-timeout")


@pytest.fixture(scope="session")
def transport(serial_port, baudrate, request):
    """Provide shared serial transport for session."""
    if request.config.getoption("--skip-hardware"):
        pytest.skip("Hardware tests skipped")

    transport = SerialTransport(serial_port, baudrate)
    try:
        transport.open()
    except Exception as e:
        pytest.skip(f"Could not open serial port {serial_port}: {e}")

    yield transport
    transport.close()


@pytest.fixture(scope="function")
def client(transport, response_timeout):
    """Provide fresh PSA client for each test."""
    client = PSAClient(transport, response_timeout=response_timeout)
    # Ensure clean state
    transport.flush()
    return client


@pytest.fixture(scope="session")
def firmware_version(transport, response_timeout):
    """Get and cache firmware version."""
    client = PSAClient(transport, response_timeout=response_timeout)
    try:
        return client.ping()
    except Exception as e:
        pytest.skip(f"Could not ping device: {e}")


# Logging setup
@pytest.fixture(scope="session", autouse=True)
def setup_logging():
    """Configure logging for tests."""
    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler('test_output.log'),
            logging.StreamHandler()
        ]
    )
    # Set psa_protocol logger level
    logging.getLogger('psa_protocol').setLevel(logging.DEBUG)
