/*
 * LOWPANConnection.cpp
 *
 * Copyright (C) 2011 IBR, TU Braunschweig
 *
 * Written-by: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "net/LOWPANConvergenceLayer.h"
#include "net/LOWPANConnection.h"
#include "core/EventDispatcher.h"
#include "core/BundleCore.h"
#include "core/TimeEvent.h"
#include <ibrcommon/net/lowpanstream.h>
#include <ibrcommon/net/lowpansocket.h>

#include <ibrcommon/Logger.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/TimeMeasurement.h>

#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <list>

#define EXTENDED_MASK	0x08
#define SEQ_NUM_MASK	0x07

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		LOWPANConvergenceLayer::LOWPANConvergenceLayer(const ibrcommon::vinterface &net, uint16_t panid, unsigned int mtu)
			: DiscoveryAgent(dtn::daemon::Configuration::getInstance().getDiscovery()),
			_net(net), _panid(panid), _ipnd_buf(BUFF_SIZE), _ipnd_buf_len(0), m_maxmsgsize(mtu), _running(false)
		{
			// convert the panid into a string
			std::stringstream ss;
			ss << _panid;

			// assign the broadcast address
			_addr_broadcast = ibrcommon::vaddress("0xffff", ss.str(), AF_IEEE802154);
		}

		LOWPANConvergenceLayer::~LOWPANConvergenceLayer()
		{
			componentDown();
		}

		dtn::core::Node::Protocol LOWPANConvergenceLayer::getDiscoveryProtocol() const
		{
			return dtn::core::Node::CONN_LOWPAN;
		}

		void LOWPANConvergenceLayer::update(const ibrcommon::vinterface &iface, DiscoveryAnnouncement &announcement) throw(dtn::net::DiscoveryServiceProvider::NoServiceHereException)
		{
			if (iface == _net)
			{
				stringstream service;

				struct sockaddr_ieee802154 address;
				address.addr.addr_type = IEEE802154_ADDR_SHORT;
				address.addr.pan_id = _panid;

				// Get address via netlink
				ibrcommon::lowpansocket::getAddress(&address.addr, iface);

				//FIXME better naming for address and panid. This will need updates to the service parser.
				service << "ip=" << address.addr.short_addr << ";port=" << _panid << ";";

				announcement.addService( DiscoveryService("lowpancl", service.str()));
			}
			else
			{
				 throw dtn::net::DiscoveryServiceProvider::NoServiceHereException();
			}
		}

		void LOWPANConvergenceLayer::send_cb(const char *buf, const size_t len, const ibrcommon::vaddress &addr)
		{
			ibrcommon::socketset socks = _vsocket.getAll();
			if (socks.size() == 0) return;
			ibrcommon::lowpansocket &sock = dynamic_cast<ibrcommon::lowpansocket&>(**socks.begin());

			// set write lock
			ibrcommon::MutexLock l(m_writelock);

			if (_addr_broadcast == addr) {
				// disable auto-ack feature for broadcast
				sock.setAutoAck(false);

				// send converted line
				sock.sendto(buf, len, 0, _addr_broadcast);

				// re-enable auto-ack feature
				sock.setAutoAck(true);
			} else {
				// send converted line
				sock.sendto(buf, len, 0, addr);
			}
		}

		void LOWPANConvergenceLayer::queue(const dtn::core::Node &node, const dtn::net::BundleTransfer &job)
		{
			const std::list<dtn::core::Node::URI> uri_list = node.get(dtn::core::Node::CONN_LOWPAN);
			if (uri_list.empty()) return;

			const dtn::core::Node::URI &uri = uri_list.front();

			std::string address;
			unsigned int pan;

			uri.decode(address, pan);

			std::stringstream ss_pan; ss_pan << pan;

			ibrcommon::vaddress addr( address, ss_pan.str(), AF_IEEE802154 );

			IBRCOMMON_LOGGER_DEBUG_TAG("LOWPANConvergenceLayer", 55) << "LOWPANConvergenceLayer::queue"<< IBRCOMMON_LOGGER_ENDL;

			ibrcommon::MutexLock lc(_connection_lock);
			LOWPANConnection *connection = getConnection(addr);
			connection->_sender.queue(job);
		}

		LOWPANConnection* LOWPANConvergenceLayer::getConnection(const ibrcommon::vaddress &addr)
		{
			// Test if connection for this address already exist
			for(std::list<LOWPANConnection*>::iterator i = ConnectionList.begin(); i != ConnectionList.end(); ++i)
			{
				LOWPANConnection &conn = dynamic_cast<LOWPANConnection&>(**i);

				IBRCOMMON_LOGGER_DEBUG_TAG("LOWPANConvergenceLayer", 55) << "Connection address: " << conn._address.toString() << IBRCOMMON_LOGGER_ENDL;

				if (conn._address == addr)
					return (*i);
			}

			// Connection does not exist, create one and put it into the list
			LOWPANConnection *connection = new LOWPANConnection(addr, (*this));

			ConnectionList.push_back(connection);
			IBRCOMMON_LOGGER_DEBUG_TAG("LOWPANConvergenceLayer", 55) << "LOWPANConvergenceLayer::getConnection "<< connection->_address.toString() << IBRCOMMON_LOGGER_ENDL;
			connection->start();
			return connection;
		}

		void LOWPANConvergenceLayer::remove(const LOWPANConnection *conn)
		{
			ibrcommon::MutexLock lc(_connection_lock);

			std::list<LOWPANConnection*>::iterator i;
			for(i = ConnectionList.begin(); i != ConnectionList.end(); ++i)
			{
				if ((*i) == conn)
				{
					ConnectionList.erase(i);
					return;
				}
			}
		}

		void LOWPANConvergenceLayer::componentUp() throw ()
		{
			// routine checked for throw() on 15.02.2013
			dtn::core::EventDispatcher<dtn::core::TimeEvent>::add(this);

			try {
				_vsocket.add(new ibrcommon::lowpansocket(_panid, _net));
				_vsocket.up();

				addService(this);
			} catch (const ibrcommon::socket_exception &ex) {
				IBRCOMMON_LOGGER_TAG("LOWPANConvergenceLayer", error) << "bind to " << _net.toString() << ":" << _panid << " failed (" << ex.what() << ")" << IBRCOMMON_LOGGER_ENDL;
			}
			
			// set component in running mode
			_running = true;
		}

		void LOWPANConvergenceLayer::componentDown() throw ()
		{
			_vsocket.destroy();
			dtn::core::EventDispatcher<dtn::core::TimeEvent>::remove(this);
			stop();
			join();
		}

		void LOWPANConvergenceLayer::sendAnnoucement(const uint16_t &sn, std::list<dtn::net::DiscoveryServiceProvider*> &providers)
		{
			IBRCOMMON_LOGGER_DEBUG_TAG("LOWPANConvergenceLayer", 60) << "LOWPAN IPND beacon send started" << IBRCOMMON_LOGGER_ENDL;

			DiscoveryAnnouncement announcement(DiscoveryAnnouncement::DISCO_VERSION_01, dtn::core::BundleCore::local);

			// set sequencenumber
			announcement.setSequencenumber(sn);

			// clear all services
			announcement.clearServices();

			// add services
			for (std::list<dtn::net::DiscoveryServiceProvider*>::iterator iter = providers.begin(); iter != providers.end(); ++iter)
			{
				dtn::net::DiscoveryServiceProvider &provider = (**iter);

				try {
					// update service information
					provider.update(_net, announcement);
				} catch (const dtn::net::DiscoveryServiceProvider::NoServiceHereException&) {

				}
			}
			// Set extended header bit. Everything else 0
			_ipnd_buf[0] = 0x08;
			// Set discovery bit in extended header
			_ipnd_buf[1] = (char)0x80;

			// serialize announcement
			stringstream ss;
			ss << announcement;

			dtn::data::Length len = ss.str().size();
			if (len > 113)
				IBRCOMMON_LOGGER_TAG("LOWPANConvergenceLayer", error) << "Discovery announcement to big (" << len << ")" << IBRCOMMON_LOGGER_ENDL;

			// copy data infront of the 2 byte header
			memcpy(&_ipnd_buf[2], ss.str().c_str(), len);

			// send out broadcast frame
			send_cb(&_ipnd_buf[0], len + 2, _addr_broadcast);
		}

		void LOWPANConvergenceLayer::componentRun() throw ()
		{
			while (_running)
			{
				try {
					ibrcommon::socketset readfds;
					_vsocket.select(&readfds, NULL, NULL, NULL);

					for (ibrcommon::socketset::iterator iter = readfds.begin(); iter != readfds.end(); ++iter) {
						ibrcommon::lowpansocket &sock = dynamic_cast<ibrcommon::lowpansocket&>(**iter);

						std::vector<char> data(m_maxmsgsize);
						char header;

						// place to store the peer address
						ibrcommon::vaddress peeraddr;

						// Receive full frame from socket
						ssize_t len = sock.recvfrom(&data[0], static_cast<size_t>(m_maxmsgsize), 0, peeraddr);

						IBRCOMMON_LOGGER_DEBUG_TAG("LOWPANConvergenceLayer", 40) << "Received IEEE 802.15.4 frame from " << peeraddr.toString() << IBRCOMMON_LOGGER_ENDL;

						// We got nothing from the socket, keep reading
						if (len <= 0)
							continue;

						// Retrieve header of frame
						header = data[0];

						// Check for extended header and retrieve if available
						if ((header & EXTENDED_MASK) && (data[1] & 0x80)) {
							DiscoveryAnnouncement announce;
							stringstream ss;
							ss.write(&data[2], len-2);
							ss >> announce;
							DiscoveryAgent::received(announce.getEID(), announce.getServices(), 30);
							continue;
						}

						ibrcommon::MutexLock lc(_connection_lock);

						// Connection instance for this address
						LOWPANConnection* connection = getConnection(peeraddr);

						// Decide in which queue to write based on the src address
						connection->getStream().queue(&data[0], len);
					}
				} catch (const ibrcommon::vsocket_interrupt&) {
					return;
				}

				yield();
			}
		}

		void LOWPANConvergenceLayer::raiseEvent(const Event *evt) throw ()
		{
			try {
				const TimeEvent &time=dynamic_cast<const TimeEvent&>(*evt);
				if (time.getAction() == TIME_SECOND_TICK)
					if (time.getTimestamp().get<size_t>() % 5 == 0)
						timeout();
			} catch (const std::bad_cast&)
			{}
		}

		void LOWPANConvergenceLayer::__cancellation() throw ()
		{
			_running = false;
			_vsocket.down();
		}

		const std::string LOWPANConvergenceLayer::getName() const
		{
			return "LOWPANConvergenceLayer";
		}
	}
}
