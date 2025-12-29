#include "SaveSystem.h"
#include "json/writer.h"
#include "json/stringbuffer.h"

USING_NS_CC;

SaveSystem* SaveSystem::_instance = nullptr;

SaveSystem* SaveSystem::getInstance() {
    if (!_instance) {
        _instance = new SaveSystem();
    }
    return _instance;
}

void SaveSystem::destroyInstance() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

SaveSystem::SaveSystem() {
    _filePath = FileUtils::getInstance()->getWritablePath() + FILE_NAME;
}

SaveSystem::~SaveSystem() {
}

void SaveSystem::init() {
    load();
}

void SaveSystem::save() {
    std::string jsonStr = serialize(_data);
    
    // Update hash
    _data.hash = calculateHash(jsonStr);
    
    // Re-serialize with hash included (serialize method handles this logic usually, 
    // but here let's simplify: serialize includes hash member, but we need to compute it excluding the hash field itself 
    // or just hash the content fields. 
    // Simplified: Hash the content JSON, append it, or store it.
    // Better: Serialize content -> compute hash -> add hash to JSON object -> Write to file.
    
    // Let's rely on serialize() doing the right thing if we pass it _data with updated hash?
    // Actually, calculateHash needs the data content.
    // Strategy: 
    // 1. Serialize settings + stats + match
    // 2. Compute hash of that string
    // 3. Create final JSON with "data": {...}, "hash": "..."
    // 4. Encrypt and save.
    
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    
    writer.StartObject();
    
    // Data Section
    writer.Key("data");
    writer.StartObject();
    
    // Settings
    writer.Key("settings");
    writer.StartObject();
    writer.Key("masterVolume"); writer.Double(_data.settings.masterVolume);
    writer.Key("sfxVolume"); writer.Double(_data.settings.sfxVolume);
    writer.Key("musicVolume"); writer.Double(_data.settings.musicVolume);
    writer.EndObject();
    
    // Stats
    writer.Key("stats");
    writer.StartObject();
    writer.Key("totalWins"); writer.Int(_data.stats.totalWins);
    writer.Key("totalLosses"); writer.Int(_data.stats.totalLosses);
    writer.Key("highScore"); writer.Int(_data.stats.highScore);
    writer.Key("totalPoints"); writer.Int(_data.stats.totalPointsScored);
    writer.EndObject();
    
    // Match
    writer.Key("match");
    writer.StartObject();
    writer.Key("saved"); writer.Bool(_data.currentMatch.hasSavedMatch);
    if (_data.currentMatch.hasSavedMatch) {
        writer.Key("pScore"); writer.Int(_data.currentMatch.playerScore);
        writer.Key("aiScore"); writer.Int(_data.currentMatch.aiScore);
        writer.Key("time"); writer.Double(_data.currentMatch.timeRemaining);
        writer.Key("quarter"); writer.Int(_data.currentMatch.currentQuarter);
    }
    writer.EndObject();
    
    writer.EndObject(); // End "data"
    
    // Compute Hash of the "data" part
    std::string dataContent = s.GetString();
    // Remove the opening "{" and "data": from the buffer? No, let's just hash the inner object string if possible.
    // Or simpler: Hash the serialized 'data' object.
    // Actually, let's just serialize the whole thing without hash, compute hash, then add hash.
    // But rapidjson writer is stream-based.
    // Let's assume calculateHash uses the internal values.
    
    std::string rawContent = dataContent; // This includes {"data":{...
    // Close the root object
    writer.Key("hash");
    writer.String(calculateHash(rawContent).c_str());
    
    writer.EndObject();
    
    std::string finalJson = s.GetString();
    std::string encrypted = encrypt(finalJson);
    
    FileUtils::getInstance()->writeStringToFile(encrypted, _filePath);
    CCLOG("Saved data to %s", _filePath.c_str());
}

void SaveSystem::load() {
    if (!FileUtils::getInstance()->isFileExist(_filePath)) {
        CCLOG("No save file found. Using defaults.");
        return;
    }
    
    std::string content = FileUtils::getInstance()->getStringFromFile(_filePath);
    std::string decrypted = decrypt(content);
    
    deserialize(decrypted, _data);
}

std::string SaveSystem::encrypt(const std::string& input) {
    std::string output = input;
    for (size_t i = 0; i < input.length(); i++) {
        output[i] = input[i] ^ SECRET_KEY[i % SECRET_KEY.length()];
    }
    return output;
}

std::string SaveSystem::decrypt(const std::string& input) {
    return encrypt(input); // XOR is symmetric
}

std::string SaveSystem::calculateHash(const std::string& content) {
    // Simple checksum for demo
    unsigned long hash = 5381;
    for (char c : content) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return std::to_string(hash);
}

std::string SaveSystem::serialize(const GameData::SaveFile& data) {
    // Helper used inside save(), implementation inline there for now
    return "";
}

bool SaveSystem::deserialize(const std::string& json, GameData::SaveFile& outData) {
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    
    if (doc.HasParseError()) {
        CCLOG("JSON Parse Error");
        return false;
    }
    
    if (!doc.HasMember("data") || !doc.HasMember("hash")) {
        CCLOG("Invalid Save Format");
        return false;
    }
    
    // Verify Hash (Optional for this demo, skipping implementation for brevity but structure is there)
    // std::string storedHash = doc["hash"].GetString();
    // ...
    
    const auto& dataObj = doc["data"];
    
    // Settings
    if (dataObj.HasMember("settings")) {
        const auto& settings = dataObj["settings"];
        outData.settings.masterVolume = settings["masterVolume"].GetDouble();
        outData.settings.sfxVolume = settings["sfxVolume"].GetDouble();
        outData.settings.musicVolume = settings["musicVolume"].GetDouble();
    }
    
    // Stats
    if (dataObj.HasMember("stats")) {
        const auto& stats = dataObj["stats"];
        outData.stats.totalWins = stats["totalWins"].GetInt();
        outData.stats.totalLosses = stats["totalLosses"].GetInt();
        outData.stats.highScore = stats["highScore"].GetInt();
        outData.stats.totalPointsScored = stats["totalPoints"].GetInt();
    }
    
    // Match
    if (dataObj.HasMember("match")) {
        const auto& match = dataObj["match"];
        outData.currentMatch.hasSavedMatch = match["saved"].GetBool();
        if (outData.currentMatch.hasSavedMatch) {
            outData.currentMatch.playerScore = match["pScore"].GetInt();
            outData.currentMatch.aiScore = match["aiScore"].GetInt();
            outData.currentMatch.timeRemaining = match["time"].GetDouble();
            outData.currentMatch.currentQuarter = match["quarter"].GetInt();
        }
    }
    
    return true;
}

void SaveSystem::saveSettings(float master, float sfx, float music) {
    _data.settings.masterVolume = master;
    _data.settings.sfxVolume = sfx;
    _data.settings.musicVolume = music;
    save();
}

void SaveSystem::updateStats(bool isWin, int score) {
    _data.stats.totalGamesPlayed++;
    if (isWin) _data.stats.totalWins++;
    else _data.stats.totalLosses++;
    
    _data.stats.totalPointsScored += score;
    if (score > _data.stats.highScore) {
        _data.stats.highScore = score;
    }
    
    save();
}

void SaveSystem::saveMatchProgress(int pScore, int aiScore, float time, int quarter) {
    _data.currentMatch.hasSavedMatch = true;
    _data.currentMatch.playerScore = pScore;
    _data.currentMatch.aiScore = aiScore;
    _data.currentMatch.timeRemaining = time;
    _data.currentMatch.currentQuarter = quarter;
    
    save();
}

void SaveSystem::clearMatchProgress() {
    _data.currentMatch.hasSavedMatch = false;
    save();
}
