/*
Copyright 2016 Thomas Jammet
mathieu.poux[a]gmail.com
jammetthomas[a]gmail.com

This file is part of Librtmfp.

Librtmfp is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Librtmfp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Librtmfp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "GroupStream.h"
#include "RTMFP.h"
#include "Mona/Util.h"

using namespace std;
using namespace Mona;

GroupStream::GroupStream(UInt16 id) : FlashStream(id) {
	DEBUG("GroupStream ", streamId, " created")
}

GroupStream::~GroupStream() {
	DEBUG("GroupStream ", streamId," deleted")
}

bool GroupStream::process(const Packet& packet, UInt64 flowId, UInt64 writerId, double lostRate) {
	if (!packet)
		return true; // Flow is closing

	UInt32 time(0);
	BinaryReader reader(packet.data(), packet.size());
	GroupStream::ContentType type = (GroupStream::ContentType)reader.read8();

	// if exception, it closes the connection, and print an ERROR message
	switch(type) {

		case GroupStream::GROUP_MEMBER: { // RTMFPSession event (TODO: see if it must be moved in FlashStream)
			string rawId(PEER_ID_SIZE + 2, '\0'), id;
			BinaryWriter writer(BIN rawId.data(), rawId.size());
			writer.write("\x21\x0F");
			reader.read(PEER_ID_SIZE, STR (rawId.data() + 2));
			String::Append(id, String::Hex(BIN rawId.data() + 2, PEER_ID_SIZE));
			onNewPeer(rawId, id);
			return true;
		}
		case GroupStream::GROUP_INIT: {
			DEBUG("GroupStream ", streamId, " - NetGroup Peer Connection (type 01)")
			if (reader.read16() != 0x4100) {
				ERROR("Unexpected format for NetGroup ID header")
				break;
			}
			string netGroupId, encryptKey, rawId;
			reader.read(0x40, netGroupId);
			if (reader.read16() != 0x2101) {
				ERROR("Unexpected format for NetGroup ID header")
				break;
			}
			reader.read(0x20, encryptKey);
			if (reader.read16() != 0x2303) {
				ERROR("Unexpected format for Peer ID header")
				break;
			}
			reader.read(PEER_ID_SIZE + 2, rawId);
			return onGroupHandshake(netGroupId, encryptKey, rawId);
		}
		case GroupStream::GROUP_DATA: {
			string value;
			if (reader.available()) {
				UInt16 size = reader.read16();
				reader.read(size, value);
			}
			DEBUG("GroupStream ", streamId, " - NetGroup data message type : ", value)
			return true;
		}
		case GroupStream::GROUP_ASK_CLOSE:
			return onGroupAskClose(streamId, flowId, writerId);
		case GroupStream::GROUP_BEGIN_NEAREST:
			onGroupBegin(streamId, flowId, writerId);
			return true;
		case GroupStream::GROUP_BEGIN:
			onGroupBegin(streamId, flowId, writerId);
			return true;
		case GroupStream::GROUP_REPORT:
			onGroupReport(reader, streamId, flowId, writerId);
			return true;
		case GroupStream::GROUP_PLAY_PUSH:
			onGroupPlayPush(reader, streamId, flowId, writerId);
			return true;
		case GroupStream::GROUP_PLAY_PULL:
			onGroupPlayPull(reader, streamId, flowId, writerId);
			return true;
		case GroupStream::GROUP_MEDIA_INFOS:
			return onGroupMedia(reader, streamId, flowId, writerId);
		case GroupStream::GROUP_FRAGMENTS_MAP:
			onFragmentsMap(reader, streamId, flowId, writerId);
			return true;
		case GroupStream::GROUP_MEDIA_DATA: {

			UInt64 counter = reader.read7BitLongValue();
			
			UInt8 mediaType = reader.read8();
			time = reader.read32();
			DEBUG("GroupStream ", streamId, " - Group media normal : counter=", counter, ", time=", time, ", type=", (mediaType == AMF::TYPE_AUDIO ? "Audio" : (mediaType == AMF::TYPE_VIDEO ? "Video" : "Unknown")))
			onFragment(type, counter, 0, mediaType, time, Packet(packet, reader.current(), reader.available()), lostRate, streamId, flowId, writerId);
			return true;
		} case GroupStream::GROUP_MEDIA_START: { // Start a splitted media sequence

			UInt64 counter = reader.read7BitLongValue();
			UInt8 splitNumber = reader.read8(); // counter of the splitted sequence
			UInt8 mediaType = splitNumber; // In rare case the splitNumber is not written so it is the media type
			if ((*reader.current()) == AMF::TYPE_AUDIO || (*reader.current()) == AMF::TYPE_VIDEO)
				mediaType = reader.read8();
			time = reader.read32();

			DEBUG("GroupStream ", streamId, " - Group media start : counter=", counter, ", time=", time, ", splitNumber=", splitNumber, ", type=", (mediaType == AMF::TYPE_AUDIO ? "Audio" : (mediaType == AMF::TYPE_VIDEO ? "Video" : "Unknown")))
			onFragment(type, counter, splitNumber, mediaType, time, Packet(packet, reader.current(), reader.available()), lostRate, streamId, flowId, writerId);
			return true;
		}
		case GroupStream::GROUP_MEDIA_NEXT: { // Continue a splitted media sequence

			UInt64 counter = reader.read7BitLongValue();
			UInt8 splitNumber = reader.read8(); // counter of the splitted sequence
			DEBUG("GroupStream ", streamId, " - Group media next : counter=", counter, ", splitNumber=", splitNumber)
			onFragment(type, counter, splitNumber, 0, 0, Packet(packet, reader.current(), reader.available()), lostRate, streamId, flowId, writerId);
			return true;
		}
		case GroupStream::GROUP_MEDIA_END: { // End of a splitted media sequence

			UInt64 counter = reader.read7BitLongValue();
			DEBUG("GroupStream ", streamId, " - Group media end : counter=", counter)
			onFragment(type, counter, 0, 0, 0, Packet(packet, reader.current(), reader.available()), lostRate, streamId, flowId, writerId);
			return true;
		}

		default:
			ERROR("GroupStream ", streamId, ", Unpacking type '", String::Format<UInt8>("%02X",(UInt8)type), "' unknown")
	}

	return false;
}
