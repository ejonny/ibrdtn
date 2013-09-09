/*
 * PrimaryBlock.cpp
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

#include "ibrdtn/data/PrimaryBlock.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/utils/Clock.h"
#include <ibrcommon/thread/MutexLock.h>

namespace dtn
{
	namespace data
	{
		Number PrimaryBlock::__sequencenumber = 0;
		Timestamp PrimaryBlock::__last_timestamp = 0;
		ibrcommon::Mutex PrimaryBlock::__sequence_lock;

		PrimaryBlock::PrimaryBlock()
		 : timestamp(0), sequencenumber(0), lifetime(3600), fragmentoffset(0), appdatalength(0)
		{
			relabel();

			// by default set destination as singleton bit
			set(DESTINATION_IS_SINGLETON, true);
		}

		PrimaryBlock::~PrimaryBlock()
		{
		}

		void PrimaryBlock::set(FLAGS flag, bool value)
		{
			procflags.setBit(flag, value);
		}

		bool PrimaryBlock::get(FLAGS flag) const
		{
			return procflags.getBit(flag);
		}

		PrimaryBlock::PRIORITY PrimaryBlock::getPriority() const
		{
			if (get(PRIORITY_BIT1))
			{
				return PRIO_MEDIUM;
			}

			if (get(PRIORITY_BIT2))
			{
				return PRIO_HIGH;
			}

			return PRIO_LOW;
		}

		void PrimaryBlock::setPriority(PrimaryBlock::PRIORITY p)
		{
			// set the priority to the real bundle
			switch (p)
			{
			case PRIO_LOW:
				set(PRIORITY_BIT1, false);
				set(PRIORITY_BIT2, false);
				break;

			case PRIO_HIGH:
				set(PRIORITY_BIT1, false);
				set(PRIORITY_BIT2, true);
				break;

			case PRIO_MEDIUM:
				set(PRIORITY_BIT1, true);
				set(PRIORITY_BIT2, false);
				break;
			}
		}

		bool PrimaryBlock::operator!=(const PrimaryBlock& other) const
		{
			return !((*this) == other);
		}

		bool PrimaryBlock::operator==(const PrimaryBlock& other) const
		{
			if (other.timestamp != timestamp) return false;
			if (other.sequencenumber != sequencenumber) return false;
			if (other.source != source) return false;
			if (other.get(PrimaryBlock::FRAGMENT) != get(PrimaryBlock::FRAGMENT)) return false;

			if (get(PrimaryBlock::FRAGMENT))
			{
				if (other.fragmentoffset != fragmentoffset) return false;
				if (other.appdatalength != appdatalength) return false;
			}

			return true;
		}

		bool PrimaryBlock::operator<(const PrimaryBlock& other) const
		{
			if (source < other.source) return true;
			if (source != other.source) return false;

			if (timestamp < other.timestamp) return true;
			if (timestamp != other.timestamp) return false;

			if (sequencenumber < other.sequencenumber) return true;
			if (sequencenumber != other.sequencenumber) return false;

			if (other.get(PrimaryBlock::FRAGMENT))
			{
				if (!get(PrimaryBlock::FRAGMENT)) return true;
				return (fragmentoffset < other.fragmentoffset);
			}

			return false;
		}

		bool PrimaryBlock::operator>(const PrimaryBlock& other) const
		{
			return !(((*this) < other) || ((*this) == other));
		}

		bool PrimaryBlock::isExpired() const
		{
			return dtn::utils::Clock::isExpired(lifetime + timestamp, lifetime);
		}

		std::string PrimaryBlock::toString() const
		{
			return dtn::data::BundleID(*this).toString();
		}

		void PrimaryBlock::relabel()
		{
			if (dtn::utils::Clock::isBad())
			{
				timestamp = 0;
			}
			else
			{
				timestamp = dtn::utils::Clock::getTime();
			}

			ibrcommon::MutexLock l(__sequence_lock);
			if (timestamp > __last_timestamp) {
				__last_timestamp = timestamp;
				__sequencenumber = 0;
			}

			sequencenumber = __sequencenumber;
			__sequencenumber++;
		}
	}
}
