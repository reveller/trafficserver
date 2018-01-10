/** @file

  SSLNextProtocolAccept

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#ifndef P_SSLNextProtocolAccept_H_
#define P_SSLNextProtocolAccept_H_

#include "P_Net.h"
#include "P_EventSystem.h"
#include "P_UnixNet.h"
#include "P_SSLNetVConnection.h"
#include "P_SSLNextProtocolSet.h"
#include "I_IOBuffer.h"

extern SSLNetVConnection * ssl_netvc_cast(int event, void *edata);

// SSLNextProtocolTrampoline is the receiver of the I/O event generated when we perform a 0-length read on the new SSL
// connection. The 0-length read forces the SSL handshake, which allows us to bind an endpoint that is selected by the
// NPN extension. The Continuation that receives the read event *must* have a mutex, but we don't want to take a global
// lock across the handshake, so we make a trampoline to bounce the event from the SSL acceptor to the ultimate session
// acceptor.
struct SSLNextProtocolTrampoline : public Continuation {
  SSLNextProtocolTrampoline(const SSLNextProtocolAccept *npn, Ptr<ProxyMutex> &mutex) : Continuation(mutex), npnParent(npn)
  {
    SET_HANDLER(&SSLNextProtocolTrampoline::ioCompletionEvent);
  }

  int
  ioCompletionEvent(int event, void *edata);
//  {
//    VIO *vio;
//    Continuation *plugin;
//    SSLNetVConnection *netvc;
//
//    vio   = static_cast<VIO *>(edata);
//    netvc = dynamic_cast<SSLNetVConnection *>(vio->vc_server);
//    ink_assert(netvc != nullptr);
//
//    switch (event) {
//    case VC_EVENT_EOS:
//    case VC_EVENT_ERROR:
//    case VC_EVENT_ACTIVE_TIMEOUT:
//    case VC_EVENT_INACTIVITY_TIMEOUT:
//      // Cancel the read before we have a chance to delete the continuation
//      netvc->do_io_read(nullptr, 0, nullptr);
//      netvc->do_io_close();
//      delete this;
//      return EVENT_ERROR;
//    case VC_EVENT_READ_COMPLETE:
//      break;
//    default:
//      return EVENT_ERROR;
//    }
//
//    // Cancel the action, so later timeouts and errors don't try to
//    // send the event to the Accept object.  After this point, the accept
//    // object does not care.
//    netvc->set_action(nullptr);
//
//    // Cancel the read before we have a chance to delete the continuation
//    netvc->do_io_read(nullptr, 0, nullptr);
//    plugin = netvc->endpoint();
//    if (plugin) {
//      send_plugin_event(plugin, NET_EVENT_ACCEPT, netvc);
//    } else if (npnParent->endpoint) {
//      // Route to the default endpoint
//      send_plugin_event(npnParent->endpoint, NET_EVENT_ACCEPT, netvc);
//    } else {
//      // No handler, what should we do? Best to just kill the VC while we can.
//      netvc->do_io_close();
//    }
//
//    delete this;
//    return EVENT_CONT;
//  }

  const SSLNextProtocolAccept *npnParent;
};

class SSLNextProtocolAccept : public SessionAccept
{
public:
  SSLNextProtocolAccept(Continuation *, bool);
  ~SSLNextProtocolAccept();

  bool accept(NetVConnection *, MIOBuffer *, IOBufferReader *);

  // Register handler as an endpoint for the specified protocol. Neither
  // handler nor protocol are copied, so the caller must guarantee their
  // lifetime is at least as long as that of the acceptor.
  bool registerEndpoint(const char *protocol, Continuation *handler);

  // Unregister the handler. Returns false if this protocol is not registered
  // or if it is not registered for the specified handler.
  bool unregisterEndpoint(const char *protocol, Continuation *handler);

  SLINK(SSLNextProtocolAccept, link);
  SSLNextProtocolSet *getProtoSet();
  SSLNextProtocolSet *cloneProtoSet();

  // noncopyable
  SSLNextProtocolAccept(const SSLNextProtocolAccept &) = delete;            // disabled
  SSLNextProtocolAccept &operator=(const SSLNextProtocolAccept &) = delete; // disabled

private:
  int mainEvent(int event, void *netvc);

  MIOBuffer *buffer; // XXX do we really need this?
  Continuation *endpoint;
  SSLNextProtocolSet protoset;
  bool transparent_passthrough;

  friend struct SSLNextProtocolTrampoline;
};

#endif /* P_SSLNextProtocolAccept_H_ */
