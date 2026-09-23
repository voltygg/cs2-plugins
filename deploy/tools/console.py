from rich.console import Console

# The same line styles as the voltmod CLI, which the deploy group does not install.
_out = Console(highlight=False, soft_wrap=True)
_err = Console(stderr=True, highlight=False, soft_wrap=True)


def section(title: str) -> None:
    _out.print(f"=== {title} ===", style="bold", markup=False)


def item(text: str) -> None:
    _out.print(f"    {text}", markup=False)


def dry(text: str) -> None:
    """A command a dry run would have sent."""
    _out.print(f"DRY: {text}", style="dim", markup=False)


def info(text: str = "") -> None:
    _out.print(text, markup=False)


def done(text: str) -> None:
    _out.print(text, style="bold green", markup=False)


def warn(text: str) -> None:
    _err.print(f"warning: {text}", style="yellow", markup=False)


def error(text: str) -> None:
    _err.print(f"error: {text}", style="bold red", markup=False)
