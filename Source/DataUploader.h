#pragma once

#include <JuceHeader.h>
#include <nlohmann/json.hpp>

#define API_ENDPOINT_URL "https://api.soundmeter.live"
#define API_UPLOAD_PATH "/api/add-points"

#define CONNECT_TIMEOUT_MS (10000)

/**
* Uploads a data packet to the server.
* Create a new instance passing an <int, float> map,
* which is a key value pair of timestamps and values.
* You can use the static member timeNow() to record the time at any point.
*
* Use getStatus() to check upload status and getError() to get an error if there was one.
*/
class DataUploader : private juce::Thread {
    using json = nlohmann::json;

public:
    DataUploader(const std::map<int, float>& points) :
        juce::Thread("DataUploader"), points(json::array()), succeeded{}, error{}
    {
        // convert data map to json format
        for (const auto& [time, value] : points) {
            this->points.push_back(
                {
                    { "timeAt", time },
                    { "value", value }
                }
            );
        }

        // start the run() method in a separate thread
        startThread();
    }

    ~DataUploader() {
        bool cleanExit = stopThread(-1);
        if (!cleanExit) DBG("WARNING: data uploader was killed prematurely.");
    }

    // execute server upload
    void run() override {
        // build request and upload data
        std::stringstream s{};
        s << API_ENDPOINT_URL << API_UPLOAD_PATH;
        juce::URL url(s.str());
        url = url.withPOSTData(json(
            {
                { "currentTime", timeNow() },
                { "points", this->points }
            }
        ).dump());

        // run request
        int status{};
        if (auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders("Content-Type: application/json\n")
            .withConnectionTimeoutMs(CONNECT_TIMEOUT_MS)
            .withStatusCode(&status)))
        { // if we got a response...
            json response = json::parse(stream->readEntireStreamAsString().toStdString());

            // handle success
            if (status == 200) {
                DBG("\n\nsuccessfully uploaded new data\n\n");
                juce::MessageManagerLock mml(this);
                if (mml.lockWasGained()) {
                    this->succeeded = true;
                }
                return;
            }

            // handle error messages provided by the server
            juce::MessageManagerLock mml(this);
            if (mml.lockWasGained()) {
                this->error = response.dump(2);
            }
            return;
        }

        // handle errors with no message/connection errors
        juce::MessageManagerLock mml(this);
        if (mml.lockWasGained()) {
            std::stringstream s{};
            if (status) s << "Unknown error, status " << status;
            else s << "Failed to connect";
            this->error = s.str();
        }
    }

    enum Status {
        /** no response received yet */
        PENDING,
        /** upload was successful */
        SUCCESS,
        /** there was an error uploading. run getError() for more information. */
        ERROR
    };
    Status getStatus() const {
        if (this->succeeded) return Status::SUCCESS;
        if (this->error.length()) return Status::ERROR;
        return Status::PENDING;
    }
    juce::String getError()const { return this->error; }

    /** [STATIC UTILITY] get the current time in seconds */
    static inline int timeNow() {
        return (int)(juce::Time().currentTimeMillis() / 1000.0);
    }

private:

    json points; // points array converted to JSON

    bool succeeded; // did upload succeed
    juce::String error; // error message if there was one
};
