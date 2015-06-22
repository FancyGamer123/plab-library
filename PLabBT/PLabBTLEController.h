/*
 * PLabBTLEController
 * Version 0.1, June, 2015
 * This library for the Arduino Uno simplify network communication for
 * Bluetooth LE HM-10 and HM11 based devices. This library extends
 * the SoftwareSerial library, and wrap a controller around AT commands.
 *
 * Created by Inge Edward Halsaunet, 2015
 * Released into the public domain
 */

#ifndef PLAB_BTLE_DEVICE_H
#define PLAB_BTLE_DEVICE_H

// Pause to assess end of AT reply when no clear distinction is inferred
#ifndef PLAB_BTLE_AT_PAUSE
#define PLAB_BTLE_AT_PAUSE 10
#endif
// Maximal number of devices a discovery can find
#ifndef PLAB_BTLE_MAXDEVICE
#define PLAB_BTLE_MAXDEVICE 6
#endif
// Maximal buffer size while doing AT commands
#ifndef PLAB_BTLE_BSIZE
#define PLAB_BTLE_BSIZE 25
#endif

#include <SoftwareSerial.h>

class PLabBTLEDevice {
	friend class PLabBTLEController;
private:
	int id = -1;
	char *name = 0;
	char *address = 0;
public:
	~PLabBTLEDevice()
	{
		delete[] name;
		delete[] address;
	}
	int id() const;
	const char *name() const;
	const char *address() const;
};

class PLabBTLEStateEngine
{
private:
	enum State {
		START, RECV_PREFIX, RECV_MSG
	};
	State state = State::Start;
	char buffer[PLAB_BTLE_BSIZE] = { 0 };
	int bLoc = 0;
	int pLoc = 0;
public:
	char *prefix = 0;
	// -1 : ERROR
	//  0 : Receiving prefix
	//  1 : Receiving message
	// On end-of-message recive (0), return length of message
	int update(char c) {
		switch (state)
		{
		case State::START:
			bLoc = 0;
			buffer[bLoc] = 0;
			if (prefix == 0)
			{
				state = State::RECV_MSG;
			}
			else
			{
				pLoc = 0;
				return prefix[pLoc++] == c ? 0 : -1;
			}
			break;
		case State::RECV_PREFIX:
			if (prefix[pLoc] == 0)
			{
				state = State::RECV_MSG;
			}
			else
			{
				return prefix[pLoc++] == c ? 0 : -1;
			}
			break;
		}
		// If we get here, we are in state RECV_MSG
		if (c == 0)
		{
			state = State::Start;
			return bLoc;
		}
		if (bLoc >= (PLAB_BTLE_BSIZE - 1))
		{
			buffer[PLAB_BTLE_BSIZE - 1] = 0;
			return -1;
		}
		buffer[bLoc++] = c;
		buffer[bLoc] = 0;
		return 1;
	}
	char *getMessage() { return buffer; }
};

class PLabBTLEController : public SoftwareSerial
{
private:
	enum State {
		INIT = 0, INIT_GETNAME, INIT_GETROLE, INIT_GETDISCNAME, INIT_WORKIMM,
		READY,
		SETTING_ROLE, SETTING_NAME, SETTING_NAME_DISCOVERY, SETTING_WORKIMM,
		DISCOVERY_RUNNING, CONNECT_RUNNING, DISCONNECTING,
		RESET_IN_PROGRESS, FACTORY_RESET_IN_PROGRESS
	};
	const char *statePrefixes = {
		0, "OK+NAME:", "OK+Get:", "OK+Set:", "OK+DIS", "OK+RESET", "OK+RENEW",
		"OK+CONN"
	};
	const int statePrefixIndex[] = {
		0, 1, 2, 2, 2,
		0,
		3, 3, 3, 3,
		4, 7, 0,
		5, 6
	};

	PLabBTLEStateEngine engine;

	state = State::INIT;

	unsigned long lastTime;

	char *name = 0;
	PLabBTLEDevice *_devices;
	bool connected, discoverNames, workImmediately, connFail = false;;
	int connectedTo = -1;
	Role role = Role::UNDEFINED;
	// Defaultconstructor should never be used
	PLabBTLEController() : SoftwareSerial(0, 1){};
	void init(long speed);
	void updateName();
public:

	enum Role { UNDEFINED = 0, PERIPHERAL = '0', CENTRAL = '1' };

	PLabBTLEController(int rx, int tx, long speed = 9600) :
		SoftwareSerial(rx, tx) {
		init(speed);
	}
	PLabBTLEController(const PLabBTLEController &other) = delete;
	~PLabBTLEController() : ~SofwareSerial() {
		delete[] name;
		delete[] _devices;
	}

	const PLabBTLEDevice &operator[](const int &index) const;

	void update();
	bool isReady() const;

	void setName(char *name) const;
	const char* getName() const;

	Role getRole() const;
	void setRole(Role r);
	bool isPeripheral() const;
	bool isCentral() const;

	bool isDiscoveringNames() const;
	void setDiscoveringNames(bool disc);

	bool isWorkingImmediately() const;
	void setWorkingImmediately(bool wi);

	void discoverDevices();
	bool isDiscoveringDevices() const;
	int devices() const;

	void connectDevice(int id);
	void connectDevice(const PLabBTLEDevice &device);
	void connectLastDevice();
	bool connectionFailed() const;
	bool isConnecting() const;
	bool isConnected() const;
	void disconnect();

	virtual int available()
	{
		if (state == State::READY)
		{
			return SofwareSerial::available();
		}
		return 0;
	}

	void reset() const;
	void factoryReset();
}
#endif
