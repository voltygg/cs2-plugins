#!/usr/bin/env python3
# vim: set ts=8 sts=4 sw=4 tw=99 et:
#
# Admin System - CS2 Metamod Plugin
# AMBuild Configuration Script
#

import os
import sys

# Add AMBuild path
if os.path.exists(os.path.join(os.path.dirname(__file__), "metamod", "third_party", "ambuild")):
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), "metamod", "third_party", "ambuild"))

from ambuild2 import run

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
    default=os.environ.get("MMSOURCE", None),
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

parser.Configure()
