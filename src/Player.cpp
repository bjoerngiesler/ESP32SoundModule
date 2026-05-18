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
//    bb::printf("Stream 0x%x started\n", stream);
}
void onEndCallback(Stream* stream) {
    //bb::printf("Stream 0x%x ended\n", stream);
}

Player::Player():
#if defined(MEASURE_THROUGHPUT)
    meas_(20, &Serial),
#endif
    finalCopier_(AUDIO_BUFFER_SIZE)
{
    pinMode(LED_BUILTIN, OUTPUT);
}

Result Player::initialize() {
    addCommand("play", "<filename>|<num>|<folder_num> <file_num>: Play the given filename or numbered file", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return Player::inst.playCmd(words, stream); }, 1, 2);
    addCommand("end", "Stop playback", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { Player::inst.stopPlayback(); return RES_OK; }, 0, 0);
    addCommand("vol", "[<volume>]: Query or set master volume", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return Player::inst.volumeCmd(words, stream); }, 0, 1);
    addCommand("siggen", "on|off|sine|square|saw|volume <num>: Control the signal generator", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return Player::inst.sigGenCmd(words, stream); }, 1, 2);
    addCommand("ls", "[<path>]: Print a list of files in <path> or in current working directory", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return FileManager::inst.lsCmd(words, stream); }, 0, 1);
    addCommand("cat", "<file>: Print contents of <file>", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return FileManager::inst.catCmd(words, stream); }, 1, 1);
    addCommand("cd", "[<path>]: Change current working directory to <path> or '/' if omitted", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return FileManager::inst.cdCmd(words, stream); }, 0, 1);
    addCommand("pwd", "Print current working directory", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return FileManager::inst.pwdCmd(words, stream); }, 0, 0);
    addCommand("mv", "<from> <to>: Rename file <from> to <to>", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return FileManager::inst.mvCmd(words, stream); }, 2, 2);
    addCommand("filemap", ": Print file map", 
               [](const std::vector<String>& words, ConsoleStream *stream)->Result { return FileManager::inst.filemapCmd(words, stream); }, 0, 0);
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
    delay_.setDuration(100);
    delay_.setFeedback(.5);

    fileVolume_.setStream(fileEffects_);
#else
    fileVolume_.setStream(catStream_);
#endif
    fileMixerIndex_ = mixer_.add(fileVolume_);

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
    fileVolume_.begin(info_);
    fileDecoder_.begin(info_);

    sineGen_.begin(info_, N_B5);
    sineGen_.setMaxAmplitudeStep(float(1<<(BITS_PER_SAMPLE-1)));
    sawGen_.begin(info_, N_B5);
    squareGen_.begin(info_, N_B5);

    isSDCardPresent_ = FileManager::inst.checkSDCardPresent();
    lastCardCheckTime_ = millis();

    return Subsystem::start();
}

Result Player::step() {
    if(paused_) return RES_OK;
    size_t bytesCopied;
    bytesCopied = finalCopier_.copy();
    LOGI("Player::step(): finalCopier copied %d bytes", bytesCopied);
    float vol = volumeMeter_.volumeRatio();
    analogWrite(LED_BUILTIN, (1-vol)*255);

    if((playMode_ == FROM_FILE && audioFile_.available() == 0) ||
       (playMode_ == FROM_URL && urlStream_.available() == 0)) {
        stopPlayback();
    }

    if((millis() - lastCardCheckTime_) > 1000) {
        FileManager::inst.checkSDCardPresent();
        lastCardCheckTime_ = millis();
    }

    bb::Runloop::runloop.excuseOverrun();
    return RES_OK;
}

Result Player::stop(ConsoleStream *stream) {
    return RES_OK;
}

void Player::setAudioInfo(AudioInfo from) {
    //bb::printf("New audioInfo: sample_rate: %d / channels: %d / bits_per_sample: %d\n", from.sample_rate, from.channels, from.bits_per_sample);
    sineGen_.setAudioInfo(from);
    squareGen_.setAudioInfo(from);
    sawGen_.setAudioInfo(from);
    sigGenStream_.setAudioInfo(from);

#if defined(EFFECTS)
    fileEffects_.setAudioInfo(from);
#endif
    fileVolume_.setAudioInfo(from);
    wav_.setAudioInfo(from);
    mp3_.setAudioInfo(from);

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

void Player::setEffects(bool onoff) {
    fileEffects_.clear();
    if(onoff) fileEffects_.addEffect(delay_);
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

    //bb::printf("Setting signal generator amplitude to %f.\n", amplitude);

    sineGen_.setAmplitude(amplitude);
    squareGen_.setAmplitude(amplitude);
    sawGen_.setAmplitude(amplitude);
}

bool Player::playFile(const String& path) {
    if(!path.endsWith(".wav") && 
       !path.endsWith(".mp3") && 
       !path.endsWith(".stream")) {
        bb::printf("Unknown file type for file '%s'\n", path.c_str());
        return false;
    }

    String p = FileManager::inst.normalizePath(path);
    File f = SD.open(p.c_str(), FILE_READ);
    if(!f) {
        LOGE("Failed to open file '%s'\n", p.c_str());
        return false;
    }
    bb::printf("Playing file '%s'\n", p.c_str());
    audioFile_ = f;
    bool success;

    if(path.endsWith(".wav")) {
        success = playEncodedStream(audioFile_, wav_);
        if(success) playMode_ = FROM_FILE;
        return success;
    } else if(path.endsWith(".mp3")) {
        success = playEncodedStream(audioFile_, mp3_);
        if(success) playMode_ = FROM_FILE;
        return success;
    } else if(path.endsWith(".stream")) {
        String url = f.readString().c_str();
        while(url.length() > 0 && (url.endsWith("\n") || url.endsWith("\r"))) url = url.substring(0, url.length()-1);
        bb::printf("Streaming from URL: '%s'\n", url.c_str());
        urlStream_.begin(url.c_str(), "audio/mp3");

        success = playEncodedStream(urlStream_, mp3_);
        if(success) playMode_ = FROM_URL;
        return success;        
    }

    return false;
}
    
bool Player::playMemFile(const MemFile& file) {
    if(file.size() == 0 || file.buffer() == nullptr) {
        bb::printf("Can't play memory file of length %d or buffer 0x%x\n", file.size(), file.buffer());
        return false;
    }
    String filename = file.filename();
    if(!filename.endsWith(".wav") && 
       !filename.endsWith(".mp3")) {
        bb::printf("Unknown file type for file '%s'\n", filename.c_str());
        return false;
    }

    if(memoryStream_ != nullptr) delete memoryStream_;
    memoryStream_ = new MemoryStream(file.buffer(), file.size());

    bool success;
    if(filename.endsWith(".wav")) {
        success = playEncodedStream(*memoryStream_, wav_);
        if(success) playMode_ = FROM_MEMORY;
        return success;
    } else if(filename.endsWith(".mp3")) {
        success = playEncodedStream(*memoryStream_, mp3_);
        if(success) playMode_ = FROM_MEMORY;
        return success;
    }

    return false;
}


bool Player::playEncodedStream(Stream& stream, AudioDecoder& dec) {
    dec.begin();
    fileDecoder_.setDecoder(&dec);
    fileDecoder_.setStream(stream);
    fileDecoder_.clearNotifyAudioChange();
    fileDecoder_.addNotifyAudioChange(Player::inst);
    fileDecoder_.begin();

    catStream_.clear();
    catStream_.add(fileDecoder_);
    catStream_.add(silenceStream_);
    catStream_.begin();

    paused_ = false;

    return true;
}

void Player::stopPlayback() {
    if(playMode() == STOPPED) return;

    if(playMode_ == FROM_URL) {
        //urlStream_.end();
        //bb::printf("Ended URL stream\n");
    }
    fileDecoder_.end();

    //bb::printf("Massaging catStream\n");
    catStream_.clear();
    catStream_.add(silenceStream_);
    catStream_.begin();

    //bb::printf("Playback stopped.\n");

    for(auto cb: onStopCallbacks_) cb();
    playMode_ = STOPPED;
}

bool Player::pausePlayback(bool onoff) {
    if(playMode_ == STOPPED) return false;

    if(paused_ == onoff) return false;
    paused_ = onoff;
    return true;
}
    
Result Player::playCmd(const std::vector<String>& args, ConsoleStream *stream) {
    if(args.size() == 1) {
        String path = FileManager::inst.normalizePath(args[0]);
        if(FileManager::inst.fileExists(path)) {
            if(playFile(path)) return RES_OK;
            return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
        }
        
        bb::printf("Not a file - trying to play '%s' as a number\n", args[0].c_str());
        String filename = FileManager::inst.filename(unsigned(args[0].toInt()));
        if(filename != "") {
            bb::printf("Resolved to '%s'\n", filename.c_str());
            if(playFile(filename)) return RES_OK;
        }
        return RES_SUBSYS_RESOURCE_NOT_AVAILABLE;
    } else if(args.size() == 2) {
        String filename = FileManager::inst.filename(unsigned(args[0].toInt()), unsigned(args[1].toInt()));
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

Result Player::volumeCmd(const std::vector<String>& args, ConsoleStream* stream) {
    if(args.size() == 0) {
        stream->printf("%.2f\n", mixerVolume_.volume());
        return RES_OK;
    }
    setOutputVolume(constrain(args[0].toFloat(), 0, 1));
    return RES_OK;
}

void Player::addOnStopCallback(std::function<bool(void)> fn) {
    onStopCallbacks_.push_back(fn);
}
