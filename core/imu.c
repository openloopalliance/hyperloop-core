#include "pod.h"
#include <termios.h>

int imuFd = -1;

typedef struct {
  // All units are based in meters
  uint32_t header;
  float x;
  float y;
  float z;
  float wx;
  float wy;
  float wz;
  uint8_t status;
  uint8_t sequence;
  float temperature;
} imu_datagram_t;

// example datagram, MSB is always printed first
// uint8_t example[32] = {
//   0xFE, 0x81, 0xFF, 0x55, // Message Header, exactly these bytes
//   0x01, 0x23, 0x45, 0x65, // X rotational (4 Bytes to form a float)
//   0x01, 0x23, 0x45, 0x65, // Y rotational (4 Bytes to form a float)
//   0x01, 0x23, 0x45, 0x65, // Z rotational (4 Bytes to form a float)
//   0x01, 0x23, 0x45, 0x65, // Z linear (4 Bytes to form a float)
//   0x01, 0x23, 0x45, 0x65, // Y linear  (4 Bytes to form a float)
//   0x01, 0x23, 0x45, 0x65, // Z linear  (4 Bytes to form a float)
//   0xEE, // Status (each bit wx, wy, wz, reserved, ax, ay, az, reserved)
//   0xFF, // sequence number (0-127 wraps)
//   0x00, 0x01 // temperature bits (UInt16)
// };

imu_datagram_t readIMUDatagram(uint64_t t) {
#ifdef TESTING
  static uint64_t i = 0;
  int32_t ax = 1000L; // 1 m/s/s

  if (t > 60ULL * 1000ULL * 1000ULL) {
    ax = -100;
  }

  printf("%llu\n", i);
  i++;
  switch ((i << 4) & 0x1) {
  case 0:
    return (imu_datagram_t){
        .x = ax, .y = 8L, .z = 0L, .wx = 0L, .wy = 0L, .wz = 0L};
  case 1:
  default:
    return (imu_datagram_t)(imu_datagram_t){
        .x = ax, .y = -8L, .z = 0L, .wx = 0L, .wy = 0L, .wz = 0L};
  }
#else
  char buf[32];

  int count = 0;
  int n = 32;
  // Force read an entire packet
  // read() is not garenteed to give you all n bytes
  while (count < n) {
    int new_count = read(imuFd, buf, n - count);

    if (new_count <= count) {
      count = -1;
      break;
    }

    count += new_count;
  }

  assert(count == 32);

  imu_datagram_t gram = {
      .header = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24),
      .wx = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24),
      .wy = buf[8] | (buf[9] << 8) | (buf[10] << 16) | (buf[11] << 24),
      .wz = buf[12] | (buf[13] << 8) | (buf[14] << 16) | (buf[15] << 24),
      .x = buf[16] | (buf[17] << 8) | (buf[18] << 16) | (buf[19] << 24),
      .y = buf[20] | (buf[21] << 8) | (buf[22] << 16) | (buf[23] << 24),
      .z = buf[24] | (buf[25] << 8) | (buf[26] << 16) | (buf[27] << 24),
      .status = buf[28],
      .sequence = buf[29],
      .temperature = buf[30] | (buf[31] << 8)};

  return gram;
#endif
}

// Connect the serial device for the IMU
int imuConnect() {
  imuFd = open(IMU_DEVICE, O_RDWR, S_IRUSR | S_IWUSR);

  if (imuFd < 0) {
    return -1;
  }

  struct termios pts;
  tcgetattr(imuFd, &pts);
  /*make sure pts is all blank and no residual values set
  safer than modifying current settings*/
  pts.c_lflag = 0; /*implies non-canoical mode*/
  pts.c_iflag = 0;
  pts.c_oflag = 0;
  pts.c_cflag = 0;

  // Baud Rate (This is super high by default)
  pts.c_cflag |= 921600;

  // 8 Data Bits
  pts.c_cflag |= CS8;

  pts.c_cflag |= CLOCAL;
  pts.c_cflag |= CREAD;

  // Set the terminal device atts
  tcsetattr(imuFd, TCSANOW, &pts);

  return imuFd;
}
/**
 * Reads data from the IMU, computes the new Acceleration, Velocity, and
 * Position state values
 */
int imuRead(pod_state_t *state) {

  static uint64_t start_time = 0;
  static uint64_t lastCheckTime = 0;

  // Intiializes local static varaibles
  if (start_time == 0) {
    lastCheckTime = getTime();
    start_time = getTime();
  }

  debug("[imuMain] Thread Start");

  uint64_t currentCheckTime = getTime(); // Same as above, assume milliseconds
  // if (currentCheckTime - start_time > 9000000ULL) { sleep(120); exit(121); }
  imu_datagram_t imu_reading = readIMUDatagram(currentCheckTime - start_time);

  assert((currentCheckTime - lastCheckTime) < INT32_MAX);

  int32_t t_usec = (int32_t)(currentCheckTime - lastCheckTime);
  lastCheckTime = currentCheckTime;

  int32_t position = getPodField(&(state->position_x));
  int32_t velocity = getPodField(&(state->velocity_x));
  int32_t acceleration = getPodField(&(state->accel_x));

  // Calculate the new_velocity (oldv + (olda + newa) / 2)

  int32_t dv = ((t_usec * ((acceleration + imu_reading.x) / 2)) / 1000000LL);

  printf("%d\n", dv);
  assertUInt32Addition(velocity, dv);
  int32_t new_velocity = (velocity + dv);

  int32_t dx = ((t_usec * ((velocity + new_velocity) / 2)) / 1000000LL);
  int32_t new_position = (position + dx);

  setPodField(&(state->position_x), new_position);
  setPodField(&(state->velocity_x), new_velocity);
  setPodField(&(state->accel_x), imu_reading.x);

  logDump(state);

  return 0;
}
