/*
 * RetransmissionExtension.cpp
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

#include "routing/RetransmissionExtension.h"
#include "routing/RequeueBundleEvent.h"
#include "core/TimeEvent.h"
#include "core/BundleCore.h"
#include "core/BundleExpiredEvent.h"
#include "net/TransferAbortedEvent.h"
#include "net/TransferCompletedEvent.h"

#include <ibrdtn/utils/Clock.h>
#include <ibrdtn/data/Exceptions.h>
#include <ibrcommon/thread/MutexLock.h>


namespace dtn
{
	namespace routing
	{
		RetransmissionExtension::RetransmissionExtension()
		{
		}

		RetransmissionExtension::~RetransmissionExtension()
		{
		}

		void RetransmissionExtension::notify(const dtn::core::Event *evt) throw ()
		{
			try {
				const dtn::core::TimeEvent &time = dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				ibrcommon::MutexLock l(_mutex);
				if (!_queue.empty())
				{
					const RetransmissionData &data = _queue.front();

					if ( data.getTimestamp() <= time.getTimestamp() )
					{
						try {
							const dtn::data::MetaBundle meta = dtn::core::BundleCore::getInstance().getStorage().get(data);

							// retransmit the bundle
							dtn::net::BundleTransfer transfer(data.destination, meta);
							dtn::core::BundleCore::getInstance().transferTo(transfer);
						} catch (const dtn::core::P2PDialupException&) {
							// do nothing here
							dtn::routing::RequeueBundleEvent::raise(data.destination, data);
						} catch (const ibrcommon::Exception&) {
							// do nothing here
							dtn::net::TransferAbortedEvent::raise(data.destination, data, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
						}

						// remove the item off the queue
						_queue.pop();
					}
				}
				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::net::TransferCompletedEvent &completed = dynamic_cast<const dtn::net::TransferCompletedEvent&>(*evt);

				// remove the bundleid in our list
				RetransmissionData data(completed.getBundle(), completed.getPeer());
				_set.erase(data);

				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::net::TransferAbortedEvent &aborted = dynamic_cast<const dtn::net::TransferAbortedEvent&>(*evt);

				// remove the bundleid in our list
				RetransmissionData data(aborted.getBundleID(), aborted.getPeer());
				_set.erase(data);

				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::routing::RequeueBundleEvent &requeue = dynamic_cast<const dtn::routing::RequeueBundleEvent&>(*evt);

				const RetransmissionData data(requeue._bundle, requeue._peer);

				ibrcommon::MutexLock l(_mutex);
				std::set<RetransmissionData>::const_iterator iter = _set.find(data);

				if (iter != _set.end())
				{
					// increment the retry counter
					RetransmissionData data2 = (*iter);
					data2++;

					// remove the item
					_set.erase(data);

					if (data2.getCount() <= 8)
					{
						// requeue the bundle
						_set.insert(data2);
						_queue.push(data2);
					}
					else
					{
						dtn::net::TransferAbortedEvent::raise(requeue._peer, requeue._bundle, dtn::net::TransferAbortedEvent::REASON_RETRY_LIMIT_REACHED);
					}
				}
				else
				{
					// queue the bundle
					_set.insert(data);
					_queue.push(data);
				}

				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::core::BundleExpiredEvent &expired = dynamic_cast<const dtn::core::BundleExpiredEvent&>(*evt);

				// delete all matching elements in the queue
				ibrcommon::MutexLock l(_mutex);

				dtn::data::Size elements = _queue.size();
				for (dtn::data::Size i = 0; i < elements; ++i)
				{
					const RetransmissionData &data = _queue.front();

					if ((dtn::data::BundleID&)data == expired._bundle)
					{
						dtn::net::TransferAbortedEvent::raise(data.destination, data, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
					}
					else
					{
						_queue.push(data);
					}

					_queue.pop();
				}

				return;
			} catch (const std::bad_cast&) { };
		}

		bool RetransmissionExtension::RetransmissionData::operator!=(const RetransmissionData &obj)
		{
			const dtn::data::BundleID &id1 = dynamic_cast<const dtn::data::BundleID&>(obj);
			const dtn::data::BundleID &id2 = dynamic_cast<const dtn::data::BundleID&>(*this);

			if (id1 != id2) return true;
			if (obj.destination != destination) return true;

			return false;
		}

		bool RetransmissionExtension::RetransmissionData::operator==(const RetransmissionData &obj)
		{
			const dtn::data::BundleID &id1 = dynamic_cast<const dtn::data::BundleID&>(obj);
			const dtn::data::BundleID &id2 = dynamic_cast<const dtn::data::BundleID&>(*this);

			if (id1 != id2) return false;
			if (obj.destination != destination) return false;

			return true;
		}

		dtn::data::Size RetransmissionExtension::RetransmissionData::getCount() const
		{
			return _count;
		}

		const dtn::data::Timestamp& RetransmissionExtension::RetransmissionData::getTimestamp() const
		{
			return _timestamp;
		}

		RetransmissionExtension::RetransmissionData& RetransmissionExtension::RetransmissionData::operator++(int)
		{
			_count++;
			_timestamp = dtn::utils::Clock::getTime();
			float backoff = ::pow((float)retry, (int)_count -1);
			_timestamp += static_cast<dtn::data::Size>(backoff);

			return (*this);
		}

		RetransmissionExtension::RetransmissionData& RetransmissionExtension::RetransmissionData::operator++()
		{
			_count++;
			_timestamp = dtn::utils::Clock::getTime();
			float backoff = ::pow((float)retry, (int)_count -1);
			_timestamp += static_cast<dtn::data::Size>(backoff);

			return (*this);
		}

		RetransmissionExtension::RetransmissionData::RetransmissionData(const dtn::data::BundleID &id, const dtn::data::EID &d, const dtn::data::Size r)
		 : dtn::data::BundleID(id), destination(d), _timestamp(0), _count(0), retry(r)
		{
			(*this)++;
		}

		RetransmissionExtension::RetransmissionData::~RetransmissionData()
		{
		}
	}
}
