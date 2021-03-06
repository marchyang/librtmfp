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

#pragma once

#include "RTMFP.h"
#include "Invoker.h"

/***************************************************
BandWriter class is used to write messages
It is implemented by FlowManager & RTMFPHandshaker
*/
struct BandWriter : virtual Mona::Object {
	BandWriter() :	_pEncoder(new RTMFP::Engine((const Mona::UInt8*)RTMFP_DEFAULT_KEY)), _pDecoder(new RTMFP::Engine((const Mona::UInt8*)RTMFP_DEFAULT_KEY)) {}
	virtual ~BandWriter() {}

	// Return the name of the session
	virtual const std::string&				name() = 0;

	// Return true if the session has failed
	virtual bool							failed()=0;

	// Return the socket object
	virtual const std::shared_ptr<Mona::Socket>&		socket(Mona::IPAddress::Family family)=0;

	// Return the decoder to start the decoding process
	std::shared_ptr<RTMFP::Engine>&			decoder() { return _pDecoder; }

protected:

	// Encryption/Decryption
	std::shared_ptr<RTMFP::Engine>			_pDecoder;
	std::shared_ptr<RTMFP::Engine>			_pEncoder;
	Mona::SocketAddress						_address;
};
