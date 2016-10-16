#include "pod.h"

/**
 * WARNING: Do Not Directly Access this struct, use getPodState() instead to
 * get a pointer to the pod state.
 */
extern pod_state_t __state;

void setPriority(pthread_t task, int priority) {
  struct sched_param sp;
  sp.sched_priority = priority;

  // Set thread scheduling mode to Round-Robin
  if (pthread_setschedparam(task, SCHED_RR, &sp)) {
    fprintf(stderr, "WARNING: failed to set thread"
                    "to real-time priority\n");
  }
}

/**
 * Wrapper for exit() function. Allows us to dump a constant state on exit
 * Note: atexit handlers don't always work (expecially if exiting in a signal
 * handler)
 */
void pod_exit(int code) {
  fprintf(stderr, "=== POD IS SHUTTING DOWN NOW! ===\n");
  exit(code);
}

void *coreMain(void *arg);
void *loggingMain(void *arg);
void *commandMain(void *arg);

void digitalWrite(int pin, int val) {}
/**
 * Panic Signal Handler.  This is only called if the shit has hit the fan
 * This function fires whenever the controller looses complete control in itself
 *
 * The controller sets the EBRAKE pins to LOW (engage) and then kills all it's
 * threads.  This is done to prevent threads from toggling the Ebrake pins OFF
 * for whatever reason.
 *
 * This is a pretty low level function because it is attempting to cut out the
 * entire controller logic and just make the pod safe
 */
void signal_handler(int sig) {
  __state.mode = Emergency;

  // Manually make the pod safe
  int ebrake_pins[] = EBRAKE_PINS;

  // Set all the ebrake pins to 0
  for (int i = 0; i < sizeof(ebrake_pins) / sizeof(int); i++) {
    fprintf(stderr, "[PANIC] Forcing Pin %d => 0\n", ebrake_pins[i]);
    digitalWrite(ebrake_pins[i], 0);
  }

  pod_exit(EXIT_FAILURE);
}

void sigpipe_handler(int sig) { warn("SIGPIPE Recieved"); }

int main() {
  int boot_sem_ret = 0;
  info("POD Booting...");
  info("Initializing Pod State");
  initializePodState();

  info("Loading POD state struct for the first time");
  pod_state_t *state = getPodState();

  info("Registering POSIX signal handlers");
  signal(POD_SIGPANIC, signal_handler);
  signal(SIGPIPE, sigpipe_handler);

  // -----------------------------------------
  // Logging - Remote Logging System
  // -----------------------------------------
  info("Starting the Logging Client Connection");
  pthread_create(&(state->logging_thread), NULL, loggingMain, NULL);

  // Wait for logging thread to connect to the logging server
  boot_sem_ret = sem_wait(state->boot_sem);
  if (boot_sem_ret != 0) {
    perror("sem_wait wait failed: ");
    pod_exit(1);
  }

  if (getPodMode() != Boot) {
    error("Remote Logging thread has requested shutdown, See log for details");
    pod_exit(1);
  }

  // -----------------------------------------
  // Commander - Remote Command Communication
  // -----------------------------------------
  info("Booting Command and Control Server");
  pthread_create(&(state->cmd_thread), NULL, commandMain, NULL);

  // Wait for command thread to start it's server
  boot_sem_ret = sem_wait(state->boot_sem);
  if (boot_sem_ret != 0) {
    perror("sem_wait wait failed: ");
    pod_exit(1);
  }

  // Assert State is still boot
  if (getPodMode() != Boot) {
    error("Command thread has requested shutdown, See log for details");
    pod_exit(1);
  }

  info("Booting Core Controller Logic Thread");
  pthread_create(&(state->core_thread), NULL, coreMain, NULL);

  // we're using the built-in linux Round Roboin scheduling
  // priorities are 1-99, higher is more important
  // important note: this is not hard real-time
  setPriority(state->core_thread, 70);
  setPriority(state->logging_thread, 10);
  setPriority(state->cmd_thread, 20);

  pthread_join(state->core_thread, NULL);
  pthread_join(state->logging_thread, NULL);
  pthread_join(state->cmd_thread, NULL);

  return 0;
}