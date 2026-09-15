"""Deploy CS2 plugin servers: panel servers over the panel API, Docker hosts over SSH.

Run from the repository root: uv run python -m deploy.tools.cli <command>
"""

import json
import shlex
import subprocess
import sys
from typing import Annotated

import typer

from deploy.tools.addons.packager import PluginPackager
from deploy.tools.config.inventory import Inventory
from deploy.tools.deployer_factory import DeployerFactory
from deploy.tools.errors import DeployError
from deploy.tools.rcon import RconClient

app = typer.Typer(help=__doc__, no_args_is_help=True, pretty_exceptions_enable=False)

ServerOption = Annotated[
    str | None, typer.Option("--server", help="Server id; default: every enabled server")
]
DryRunOption = Annotated[bool, typer.Option("--dry-run", help="Show changes without making them")]
PluginsArgument = Annotated[
    list[str] | None, typer.Argument(help="Plugins to package; default: every inventory plugin")
]


@app.command()
def plan(server: ServerOption = None) -> None:
    """Print what a deploy covers, as GitHub output lines."""
    inventory = Inventory.load()
    servers = inventory.select(server)
    matrix = [{"id": item.id, "environment": item.environment} for item in servers]
    print(f"servers={json.dumps(matrix, separators=(',', ':'))}")
    print(f"docker={'true' if any(item.kind == 'docker' for item in servers) else 'false'}")
    print(f"runtime_image={inventory.runtime_image}")


@app.command()
def package(plugins: PluginsArgument = None) -> None:
    """Stage the Linux build of each plugin under package/."""
    packager = PluginPackager()
    for plugin in plugins or Inventory.load().plugins:
        packager.package(plugin)


@app.command()
def deploy(server: ServerOption = None, dry_run: DryRunOption = False) -> None:
    """Install plugins and settings, then restart the servers."""
    for deployer in DeployerFactory().for_servers(server, dry_run=dry_run):
        deployer.deploy()


@app.command()
def update(server: ServerOption = None, dry_run: DryRunOption = False) -> None:
    """Restart servers so they install the latest CS2 build."""
    for deployer in DeployerFactory().for_servers(server, dry_run=dry_run):
        deployer.update()


@app.command()
def cleanup(
    server: Annotated[str, typer.Option("--server")],
    yes: Annotated[bool, typer.Option("--yes", help="Confirm the deletion")] = False,
    dry_run: DryRunOption = False,
) -> None:
    """Remove a Docker host's containers, deploy files, CS2 install and runtime images."""
    if not yes and not dry_run:
        raise DeployError("cleanup deletes the server's files; pass --yes, or --dry-run to preview")
    DeployerFactory().for_server(server, dry_run=dry_run).cleanup()


@app.command()
def rcon(
    commands: Annotated[list[str], typer.Argument(help="Console commands, each quoted")],
    server: ServerOption = None,
    instance: Annotated[str | None, typer.Option("--instance")] = None,
) -> None:
    """Run console commands on a live instance."""
    deployer = DeployerFactory().for_server(server)
    target = deployer.server.instance(instance)
    password = deployer.env.require("RCON_PASSWORD")
    with deployer.rcon_address(target) as (host, port), RconClient(host, port, password) as client:
        for command in commands:
            print(f"### {command}")
            if response := client.execute(command):
                print(response)


@app.command()
def tunnel(
    server: ServerOption = None,
    local_port: Annotated[int, typer.Option("--local-port")] = 5433,
    db_host: Annotated[str, typer.Option("--db-host", help="As seen from the host")] = "localhost",
    db_port: Annotated[int, typer.Option("--db-port")] = 5432,
) -> None:
    """Forward a local port to a Docker host's database until Ctrl-C."""
    DeployerFactory().for_server(server).tunnel_database(local_port, db_host, db_port)


def main() -> None:
    try:
        app()
    except DeployError as error:
        sys.exit(f"ERROR: {error}")
    except subprocess.CalledProcessError as error:
        sys.exit(f"ERROR: `{shlex.join(error.cmd)}` exited with code {error.returncode}")


if __name__ == "__main__":
    main()
