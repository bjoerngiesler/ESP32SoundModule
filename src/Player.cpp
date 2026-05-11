#include "Player.h"
#include "Settings.h"
#include "FileManager.h"
#include <AudioTools/CoreAudio/BaseStream.h>

Player Player::inst;

#define EFFECTS

#if defined(MEASURE_THROUGHPUT)
#define OUTPUT_STREAM meas_
#else
#if defined(OUTPUT_CSV)
#define OUTPUT_STREAM csv_
#else
#define OUTPUT_STREAM i2s_
#endif
#endif

void onBeginCallback(Stream* stream) {
    bb::printf("Stream 0x%x started\n", stream);
}
void onEndCallback(Stream* stream) {
    bb::printf("Stream 0x%x ended\n", stream);
}

Player::Player():
#if defined(MEASURE_THROUGHPUT)
    meas_(20, &Serial),
#endif
    fileBuffer_(2*AUDIO_BUFFER_SIZE),
    finalCopier_(AUDIO_BUFFER_SIZE)
{
    pinMode(LED_BUILTIN, OUTPUT);
}

Result Player::initialize() {
    addCommand("play", "<filename>|<num>|<folder_num> <file_num>: Play the given filename or numbered file", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return Player::inst.playCmd(words, stream); }, 1, 2);
    addCommand("siggen", "on|off|sine|square|saw|volume <num>: Control the signal generator", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return Player::inst.sigGenCmd(words, stream); }, 1, 2);
    addCommand("ls", "<path>: Print a list of files in <path>", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return FileManager::inst.lsCmd(words, stream); }, 1, 1);
    addCommand("cat", "<file>: Print contents of <file>", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return FileManager::inst.catCmd(words, stream); }, 1, 1);
    return Subsystem::initialize("player", "Main Player subsystem", "");
}

Result Player::start(ConsoleStream *stream) {
    pinMode(P_I2S_BCK, OUTPUT);
    pinMode(P_I2S_DATA_OUT, OUTPUT);
    pinMode(P_I2S_WS, OUTPUT);

    AudioToolsLogger.begin(Serial, AUDIO_TOOLS_LOG_LEVEL);

    // set up signal path back to front.
    // i2s_ takes its data from finalCopier_ => no stream connection.
    auto cfg = i2s_.defaultConfig(TX_MODE);
    cfg.setAudioInfo(info_);
    cfg.pin_data = P_I2S_DATA_OUT;
    cfg.pin_ws = P_I2S_WS;
    cfg.pin_bck = P_I2S_BCK;
    cfg.is_master = true;
    i2s_.begin(cfg);
    
#if defined(MEASURE_THROUGHPUT)
    meas_.setStream(volumeMeter_);
#endif
    volumeMeter_.setStream(mixerVolume_);
    mixerVolume_.setStream(mixer_);

    sigGenStream_.setInput(sineGen_);

    catStream_.clear();
    catStream_.add(silenceStream_);
    catStream_.setOnBeginCallback(onBeginCallback);
    catStream_.setOnEndCallback(onEndCallback);

#if defined(EFFECTS)    
    fileEffects_.setStream(catStream_);
    fileEffects_.addEffect(delay_);
    delay_.setDuration(100);
    delay_.setFeedback(.5);

    fileVolume_.setStream(fileEffects_);
#else
    fileVolume_.setStream(catStream_);
#endif
    fileMixerIndex_ = mixer_.add(fileVolume_);

    fileDecoder_.addNotifyAudioChange(*this);

#if defined(MEASURE_THROUGPUT)
    finalCopier_.begin(i2s_, meas_);
    meas_.begin(info_);
#else
    finalCopier_.begin(i2s_, volumeMeter_);
#endif
    volumeMeter_.begin(info_);
    mixerVolume_.begin(info_);
    mixer_.begin(info_);
    sigGenStream_.begin(info_);

    silenceStream_.begin();
    catStream_.begin();
#if defined(EFFECTS)
    fileEffects_.begin(info_);
#endif
    fileBuffer_.begin();
    fileVolume_.begin(info_);
    fileDecoder_.begin(info_);

    sineGen_.begin(info_, N_B5);
    sineGen_.setMaxAmplitudeStep(float(1<<(BITS_PER_SAMPLE-1)));
    sawGen_.begin(info_, N_B5);
    squareGen_.begin(info_, N_B5);

    return Subsystem::start();
}

Result Player::step() {
    size_t bytesCopied;
    bytesCopied = finalCopier_.copy();
    LOGI("Player::step(): finalCopier copied %d bytes", bytesCopied);
    float vol = volumeMeter_.volumeRatio();
    analogWrite(LED_BUILTIN, (1-vol)*255);
    //printf("Volume: %f\n", vol);
    bb::printf("%d bytes available in file decoder\n", fileDecoder_.available()); 
    bb::printf("%d bytes available in file\n", audioFile_.available()); 
    if(audioFile_.available() == 0) {
        catStream_.clear();
        fileDecoder_.end();
        catStream_.add(silenceStream_);
        catStream_.begin();
    }

#if 0
    if(isPlaying()) {
        bb::printf("Audio file available: %d\n", audioFile_.available());
        if(audioFile_.available() == 0) {
            stopPlayback();
        }
    }
#endif

    bb::Runloop::runloop.excuseOverrun();
    return RES_OK;
}

Result Player::stop(ConsoleStream *stream) {
    return RES_OK;
}

Result Player::handleConsoleCommand(const std::vector<String>& words, ConsoleStream *stream) {
    if(words[0] == "play") {

    }

    return Subsystem::handleConsoleCommand(words, stream);
}

void Player::setAudioInfo(AudioInfo from) {
    LOGI("New audioInfo: sample_rate: %d / channels: %d / bits_per_sample: %d\n", from.sample_rate, from.channels, from.bits_per_sample);
    sineGen_.setAudioInfo(from);
    squareGen_.setAudioInfo(from);
    sawGen_.setAudioInfo(from);
    sigGenStream_.setAudioInfo(from);

#if defined(EFFECTS)
    fileEffects_.setAudioInfo(from);
#endif
    fileVolume_.setAudioInfo(from);
    fileBuffer_.setAudioInfo(from);

    mixer_.setAudioInfo(from);
    mixerVolume_.setAudioInfo(from);
    volumeMeter_.setAudioInfo(from);
#if defined(MEASURE_THROUGHPUT)
    meas_.setAudioInfo(from);
#endif
    i2s_.setAudioInfo(from);

    setSignalGeneratorVolume(sigGenVolume_);

    info_ = from;
}

void Player::setSignalGeneratorEnabled(bool yn) {
    if(yn == false && signalMixerIndex_ >= 0) {
        bb::printf("Disabling signal generator.\n");
        mixer_.remove(signalMixerIndex_);
        signalMixerIndex_ = -1;
    } else if(yn == true && signalMixerIndex_ < 0) {
        bb::printf("Enabling signal generator.\n");
        signalMixerIndex_ = mixer_.add(sigGenStream_);
        sigGenStream_.begin();
    }
}

void Player::setSignalGeneratorWaveform(Waveform wf) {
    switch(wf) {
    case SINE:
    default:
        sigGenStream_.setInput(sineGen_);
        break;
    case SQUARE:
        sigGenStream_.setInput(squareGen_);
        break;
    case SAWTOOTH:
        sigGenStream_.setInput(sawGen_);
        break;
    }
}

void Player::setSignalGeneratorFrequency(float freq) {
    sineGen_.setFrequency(freq);
    squareGen_.setFrequency(freq);
    sawGen_.setFrequency(freq);
}

void Player::setSignalGeneratorVolume(float volume) {
    sigGenVolume_ = volume;                   // input is 0..1
    float amplitude = volume * ((1<<(info_.bits_per_sample-1))-1); // but actual amplitude is -INT16_MAX..INT16_MAX or whatever

    bb::printf("Setting signal generator amplitude to %f.\n", amplitude);

    sineGen_.setAmplitude(amplitude);
    squareGen_.setAmplitude(amplitude);
    sawGen_.setAmplitude(amplitude);
}

bool Player::playFile(const std::string& path) {
    std::string p = std::string("/")+path;
    audioFile_ = SD.open(p.c_str(), FILE_READ);
    if(!audioFile_) {
        LOGE("Failed to open file '%s'\n", p.c_str());
        return false;
    }
    bb::printf("Playing file '%s'\n", p.c_str());
    if(path.rfind(".wav") != std::string::npos) {
        wav_.begin();
        fileDecoder_.setDecoder(&wav_);
        fileDecoder_.setStream(audioFile_);
        fileDecoder_.begin();

        bb::printf("Clearing cat stream\n");
        catStream_.clear();
        bb::printf("Adding file decoder 0x%x\n", &fileDecoder_);
        catStream_.add(fileDecoder_);
        bb::printf("Adding silence stream 0x%x\n", &silenceStream_);
        catStream_.add(silenceStream_);
        bb::printf("Beginning cat stream\n");
        catStream_.begin();

        // fileMixerIndex_ = mixer_.add(fileVolume_);
        // bb::printf("Mixer index assigned: %d\n", fileMixerIndex_);
    } else if(path.rfind(".mp3") != std::string::npos) {
        mp3_.begin();
        fileDecoder_.setDecoder(&mp3_);
        fileDecoder_.setStream(audioFile_);
        fileDecoder_.begin();

        bb::printf("Clearing cat stream\n");
        catStream_.clear();
        bb::printf("Adding file decoder\n");
        catStream_.add(fileDecoder_);
        bb::printf("Adding silence stream\n");
        catStream_.add(silenceStream_);
        bb::printf("Beginning cat stream\n");
        catStream_.begin();
        
        // fileMixerIndex_ = mixer_.add(fileVolume_);
        // bb::printf("Mixer index assigned: %d\n", fileMixerIndex_);
    } else if(path.rfind(".stream") != std::string::npos) {
        std::string url = audioFile_.readString().c_str();
        while(url.length() > 0 && (url.back() == '\n' || url.back() == '\r')) url.pop_back(); // Trim trailing newlines
        bb::printf("Streaming from URL: '%s'\n", url.c_str());
        urlStream_.begin(url.c_str(), "audio/mp3");

        mp3_.begin();
        fileDecoder_.setDecoder(&mp3_);
        fileDecoder_.setStream(urlStream_);
        fileDecoder_.begin();

        bb::printf("Clearing cat stream\n");
        catStream_.clear();
        bb::printf("Adding file decoder\n");
        catStream_.add(fileDecoder_);
        bb::printf("Adding silence stream\n");
        catStream_.add(silenceStream_);
        bb::printf("Beginning cat stream\n");
        catStream_.begin();

        // fileMixerIndex_ = mixer_.add(fileVolume_);
        // bb::printf("Mixer index assigned: %d\n", fileMixerIndex_);
    } else {
        Serial.println("Unsupported file type");
        audioFile_.close();
        return false;
    }

    return true;
}

void Player::stopPlayback() {

    bb::printf("* Clearing cat stream\n");
    catStream_.clear();
    bb::printf("* Adding silence stream\n");
    catStream_.add(silenceStream_);
    bb::printf("* Beginning cat stream\n");
    catStream_.begin();

    fileDecoder_.end();

    // if(fileMixerIndex_ < 0) return; // not playing
    // bb::printf("Removing file mixer index %d from mixer\n", fileMixerIndex_);
    // mixer_.remove(fileMixerIndex_);
    // bb::printf("Mixer channels now: %d\n", mixer_.size());
    // fileMixerIndex_ = -1;
}
    
bool Player::isPlaying() {
    if(fileDecoder_.available() != 0) return false;
    return true;
}

Result Player::playCmd(const std::vector<String>& args, ConsoleStream *stream) {
    if(args.size() == 1) {
        if(FileManager::inst.fileExists(args[0].c_str())) {
            if(playFile(args[0].c_str())) return RES_OK;
            return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
        }
        
        bb::printf("Not a file - trying to play '%s' as a number\n", args[0].c_str());
        std::string filename = FileManager::inst.filename(unsigned(args[0].toInt()));
        if(filename != "") {
            bb::printf("Resolved to '%s'\n", filename.c_str());
            if(playFile(filename)) return RES_OK;
        }
        return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
    } else if(args.size() == 2) {
        std::string filename = FileManager::inst.filename(unsigned(args[0].toInt()), unsigned(args[1].toInt()));
        if(filename != "") {
            bb::printf("Resolved to '%s'\n", filename.c_str());
            if(playFile(filename)) return RES_OK;
        }
        return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
    }

    return RES_CMD_FAILURE;
}

Result Player::sigGenCmd(const std::vector<String>& args, ConsoleStream *stream) {
    if(args.size() == 1) {
        if(args[0] == "on") {
            setSignalGeneratorEnabled(true);
            return RES_OK;
        } else if(args[0] == "off") {
            setSignalGeneratorEnabled(false);
            return RES_OK;
        } else if(args[0] == "sine") {
            setSignalGeneratorWaveform(SINE);
            setSignalGeneratorEnabled(true);
            return RES_OK;
        }
        else if(args[0] == "square") {
            setSignalGeneratorWaveform(SQUARE);
            setSignalGeneratorEnabled(true);
            return RES_OK;
        }
        else if(args[0] == "saw") {
            setSignalGeneratorWaveform(SAWTOOTH);
            setSignalGeneratorEnabled(true);
            return RES_OK;
        }
        
        return RES_CMD_INVALID_ARGUMENT;
    }

    if(args.size() == 2) {
        if(args[0] != "volume") return RES_CMD_INVALID_ARGUMENT;
        setSignalGeneratorVolume(args[1].toFloat());
        setSignalGeneratorEnabled(true);
        return RES_OK;
    }

    return RES_CMD_INVALID_ARGUMENT;
}
