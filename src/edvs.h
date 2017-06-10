#ifndef LIBCAER_SRC_EDVS_H_
#define LIBCAER_SRC_EDVS_H_

#include "devices/edvs.h"
#include "ringbuffer/ringbuffer.h"
#include <libserialport.h>
#include <stdatomic.h>

#if defined(HAVE_PTHREADS)
#include "c11threads_posix.h"
#endif

#define MAX_THREAD_NAME_LENGTH 15

#define EDVS_DEVICE_NAME "eDVS4337"

#define EDVS_ARRAY_SIZE_X 128
#define EDVS_ARRAY_SIZE_Y 128

#define EDVS_EVENT_TYPES 2

#define EDVS_POLARITY_DEFAULT_SIZE 4096
#define EDVS_SPECIAL_DEFAULT_SIZE 128

#define BIAS_NUMBER 12
#define BIAS_LENGTH 3

struct serial_state {
	// Serial Device State
	struct sp_port *serialPort;
	// Serial thread state
	thrd_t serialThread;
	atomic_bool serialThreadRun;
	// Serial Data Transfers
	atomic_uint_fast32_t serialReadSize;
	// Serial Data Transfers shutdown callback
	void (*serialShutdownCallback)(void *serialShutdownCallbackPtr);
	void *serialShutdownCallbackPtr;
};

struct edvs_state {
	// Per-device log-level
	atomic_uint_fast8_t deviceLogLevel;
	// Data Acquisition Thread -> Mainloop Exchange
	RingBuffer dataExchangeBuffer;
	atomic_uint_fast32_t dataExchangeBufferSize; // Only takes effect on DataStart() calls!
	atomic_bool dataExchangeBlocking;
	atomic_bool dataExchangeStartProducers;
	atomic_bool dataExchangeStopProducers;
	void (*dataNotifyIncrease)(void *ptr);
	void (*dataNotifyDecrease)(void *ptr);
	void *dataNotifyUserPtr;
	// Serial Device State
	struct serial_state serialState;
	// Timestamp fields
	int32_t wrapOverflow;
	int32_t wrapAdd;
	int32_t lastTimestamp;
	int32_t currentTimestamp;
	uint16_t lastShortTimestamp; // For wrap detection.
	// Packet Container state
	caerEventPacketContainer currentPacketContainer;
	atomic_uint_fast32_t maxPacketContainerPacketSize;
	atomic_uint_fast32_t maxPacketContainerInterval;
	int64_t currentPacketContainerCommitTimestamp;
	// Polarity Packet State
	caerPolarityEventPacket currentPolarityPacket;
	int32_t currentPolarityPacketPosition;
	// Special Packet State
	caerSpecialEventPacket currentSpecialPacket;
	int32_t currentSpecialPacketPosition;
	// Camera bias and settings memory (for getter operations)
	uint8_t biases[BIAS_NUMBER][BIAS_LENGTH];
	atomic_bool dvsRunning;
	atomic_bool dvsTSReset;
};

typedef struct edvs_state *edvsState;

struct edvs_handle {
	uint16_t deviceType;
	// Information fields
	struct caer_edvs_info info;
	// State for data management.
	struct edvs_state state;
};

typedef struct edvs_handle *edvsHandle;

caerDeviceHandle edvsOpen(uint16_t deviceID, const char *serialPortName, uint32_t serialBaudRate);
bool edvsClose(caerDeviceHandle handle);

bool edvsSendDefaultConfig(caerDeviceHandle handle);
// Negative addresses are used for host-side configuration.
// Positive addresses (including zero) are used for device-side configuration.
bool edvsConfigSet(caerDeviceHandle handle, int8_t modAddr, uint8_t paramAddr, uint32_t param);
bool edvsConfigGet(caerDeviceHandle handle, int8_t modAddr, uint8_t paramAddr, uint32_t *param);

bool edvsDataStart(caerDeviceHandle handle, void (*dataNotifyIncrease)(void *ptr),
	void (*dataNotifyDecrease)(void *ptr), void *dataNotifyUserPtr, void (*dataShutdownNotify)(void *ptr),
	void *dataShutdownUserPtr);
bool edvsDataStop(caerDeviceHandle handle);
caerEventPacketContainer edvsDataGet(caerDeviceHandle handle);

#endif /* LIBCAER_SRC_EDVS_H_ */