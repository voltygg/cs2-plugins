#!/usr/bin/env python3
# vim: set ts=8 sts=4 sw=4 tw=99 et:
#
# Admin System - CS2 Metamod Plugin
# AMBuild Configuration Script
#

import sys
import os

# Add AMBuild path
if os.path.exists(
    os.path.join(os.path.dirname(__file__), "metamod", "third_party", "ambuild")
):
    sys.path.insert(
        0, os.path.join(os.path.dirname(__file__), "metamod", "third_party", "ambuild")
    )

from ambuild2 import run


def make_objdir_name(p):
    return "obj-" + p.target_platform


parser = run.BuildParser(sourcePath=sys.path[0], api="2.2")

# HL2SDK path options
parser.options.add_argument(
    "--hl2sdk-root",
    type=str,
    dest="hl2sdk_root",
    default=os.environ.get("HL2SDKCS2", None),
    help="Root path to HL2SDK-CS2",
)

parser.options.add_argument(
    "--mms-path",
    type=str,
    dest="mms_path",
    default=os.environ.get("MMSOURCE112", None),
    help="Path to Metamod:Source",
)

# Build options
parser.options.add_argument(
    "--enable-debug",
    action="store_true",
    dest="debug",
    default=False,
    help="Enable debugging symbols",
)

parser.options.add_argument(
    "--enable-optimize",
    action="store_true",
    dest="optimize",
    default=True,
    help="Enable optimization",
)

# Visual Studio generation
parser.options.add_argument(
    "--gen", type=str, dest="generator", default=None, help="Build file generator (vs)"
)

parser.options.add_argument(
    "--vs-version",
    type=str,
    dest="vs_version",
    default="18",
    help="Visual Studio version (17 = 2022, 18 = 2026)",
)

parser.Configure(make_objdir_name)
