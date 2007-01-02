#!/usr/bin/env python
#
# custom_rhino.jar from:
#   http://dojotoolkit.org/svn/dojo/buildscripts/lib/custom_rhino.jar
#

import os
import re
import sys
import shutil
import subprocess
mk = file('PlotKit/PlotKit.js').read()
if len(sys.argv) > 1:
    outf = sys.stdout
else:
    outf = file('PlotKit/PlotKit_Packed.js', 'w')
VERSION = re.search(
    r"""(?mxs)PlotKit.PlotKit.VERSION\s*=\s*['"]([^'"]+)""",
    mk
).group(1)
if len(sys.argv) > 1:
    SUBMODULES = sys.argv[1:]
else:
    SUBMODULES = map(str.strip, re.search(
        r"""(?mxs)PlotKit.PlotKit.SUBMODULES\s*=\s*\[([^\]]+)""",
        mk
    ).group(1).replace(' ', '').replace('"', '').split(','))

alltext = '\n'.join(
    [file('PlotKit/%s.js' % m).read() for m in SUBMODULES])

tf = file('_scratch.js', 'w')
tf.write(alltext)
tf.flush()

p = subprocess.Popen(
    ['java', '-jar', 'scripts/custom_rhino.jar', '-c', tf.name],
    stdout=subprocess.PIPE,
)
print >>outf, """/***

    PlotKit.PlotKit %(VERSION)s : PACKED VERSION

    THIS FILE IS AUTOMATICALLY GENERATED.  If creating patches, please
    diff against the source tree, not this file.

    For more information, <http://www.liquidx.net/plotkit/>.
    
    Copyright (c) 2006. Alastair Tse.

***/
""" % locals()
shutil.copyfileobj(p.stdout, outf)
outf.write('\n')
outf.flush()
outf.close()
tf.close()
os.remove(tf.name)
