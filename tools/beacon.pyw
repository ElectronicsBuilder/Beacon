"""Double-click to start the ScoreBoard control panel (no console window)."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from scoreboard.gui import main  # noqa: E402

main()
