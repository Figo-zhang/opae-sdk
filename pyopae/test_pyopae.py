# Copyright(c) 2018, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import uuid
import random
import opae

NLB0 = "d8424dc4-a4a3-c413-f89e-433683f9040b"


def test_guid():
    props = opae.properties()
    props.guid = NLB0
    guid_str = props.guid
    print guid_str
    guid = uuid.UUID(guid_str)
    assert str(guid).lower() == NLB0


def test_enumerate():
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks


def test_open():
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None


def test_mmio():
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None
    dummy_values = dict([(offset, random.randint(0, 1000))
                         for offset in range(24, 4096, 8)])

    for key, value in dummy_values.iteritems():
        resource.write_csr64(key, value)
        assert resource.read_csr64(key) == value
        resource.write_csr64(key, 0)
        assert resource.read_csr64(key) == 0


def test_buffer():
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None
    buf = opae.dma_buffer.allocate(resource, 1024)
    print buf.buffer()


def test_hellofpga():
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None
    buf = opae.dma_buffer.allocate(resource, 1024)
    assert buf is not None

def test_close():
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None
    result = resource.close()
    assert result == opae.CLOSED

def test_version():
    ver = opae.version()
    assert ver == (0, 13, 1)

def test_event():
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None
    event = opae.event.register_event(resource, opae.fpga_event_type.ERROR, 0)
    assert event is not None

if __name__ == "__main__":
    test_mmio()
    test_buffer()