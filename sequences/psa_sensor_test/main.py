"""CLI entry point for PSA Sensor Test sequence."""

import asyncio
import sys

# Fix for Windows asyncio compatibility
if sys.platform == "win32":
    asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())

from .sequence import PSASensorTestSequence

if __name__ == "__main__":
    exit(PSASensorTestSequence.run_from_cli())
