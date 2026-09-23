#pragma once

#include <Arduino.h>
#include <FS.h>

// One CSV per boot (session_<boot>.csv) plus an always-on raw log
// (raw_<boot>.log) of every unfiltered adapter reply, so anything the decoders
// don't understand yet can be analysed later without another drive.
// Backend is internal flash (LittleFS) or an SD card, chosen in config.h.
class SessionStore {
public:
    bool begin(uint32_t bootNumber);
    const char *backend() const { return backend_; }
    void writeRow(uint32_t ms, double unixSec, const float *values, size_t count);
    // Queues one raw exchange; flushRaw() writes the queue to storage.
    void queueRaw(uint32_t ms, const char *header, const char *request, const String &reply);
    void flushRaw();
    bool healthy() const { return ok_; }
    fs::FS *fs() const { return fs_; }
    const String &dir() const { return dir_; }
    uint64_t freeBytes() const;
    // Serial maintenance: print every file (for retrieving flash logs) / erase them.
    void dumpAll(Print &out);
    // Prints the header and the last `rows` rows of session_<boot>.csv.
    void tailSession(Print &out, uint32_t boot, int rows);
    void eraseAll();

private:
    fs::FS *fs_ = nullptr;
    File csv_, raw_;
    String rawQueue_;
    String dir_;  // "" for flash, "/obd" on SD
    uint32_t boot_ = 0;
    const char *backend_ = "none";
    bool ok_ = false;
    bool spaceLow();
};
