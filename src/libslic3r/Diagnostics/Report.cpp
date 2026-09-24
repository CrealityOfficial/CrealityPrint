#include "Report.hpp"

#include "Hasher.hpp"

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

#include <boost/nowide/fstream.hpp>
#include <nlohmann/json.hpp>

namespace Slic3r::Diagnostics {
namespace {

using json = nlohmann::ordered_json;

bool sample_less(const Sample &lhs, const Sample &rhs)
{
    if (lhs.scope.object_id != rhs.scope.object_id)
        return lhs.scope.object_id < rhs.scope.object_id;
    if (lhs.scope.invocation_id != rhs.scope.invocation_id)
        return lhs.scope.invocation_id < rhs.scope.invocation_id;
    return lhs.fingerprint < rhs.fingerprint;
}

bool error_less(const Error &lhs, const Error &rhs)
{
    if (lhs.scope.plate_id != rhs.scope.plate_id)
        return lhs.scope.plate_id < rhs.scope.plate_id;
    if (lhs.scope.object_id != rhs.scope.object_id)
        return lhs.scope.object_id < rhs.scope.object_id;
    if (lhs.scope.invocation_id != rhs.scope.invocation_id)
        return lhs.scope.invocation_id < rhs.scope.invocation_id;
    if (lhs.path != rhs.path)
        return lhs.path < rhs.path;
    return lhs.message < rhs.message;
}

std::string aggregate(const std::string &path, std::vector<Sample> samples)
{
    std::sort(samples.begin(), samples.end(), sample_less);
    Hasher hasher;
    hasher.add_string(path);
    hasher.add_unsigned(samples.size());

    for (const Sample &sample : samples) 
    {
        hasher.add_unsigned(sample.scope.object_id);
        hasher.add_unsigned(sample.scope.invocation_id);
        hasher.add_string(sample.fingerprint);
    }
    return hasher.digest();
}

json serialize_indexed_samples(const std::vector<Sample> &samples)
{
    struct IndexedSample
    {
        const Sample *sample;
        size_t        sequence_index;
    };

    std::map<size_t, size_t> sequence_indices;
    std::vector<IndexedSample> indexed_samples;
    indexed_samples.reserve(samples.size());
    for (const Sample &sample : samples) {
        if (sample.scope.invocation_id != 0)
            indexed_samples.push_back({&sample, sequence_indices[sample.scope.object_id]++});
    }
    std::sort(indexed_samples.begin(), indexed_samples.end(),
              [](const IndexedSample &lhs, const IndexedSample &rhs) {
                  return sample_less(*lhs.sample, *rhs.sample);
              });

    json result = json::array();
    for (const IndexedSample &indexed_sample : indexed_samples) {
        const Sample &sample = *indexed_sample.sample;
        json item = {
            {"sample_id", sample.scope.invocation_id - 1},
            {"sequence_index", indexed_sample.sequence_index},
            {"fingerprint", sample.fingerprint}
        };
        if (sample.scope.object_id != no_object)
            item["object_id"] = sample.scope.object_id;
        result.push_back(std::move(item));
    }
    return result;
}

} // namespace

void Report::begin_plate(int plate_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_plates.try_emplace(plate_id);
}

void Report::record(const Scope &scope, std::string path,
                    std::string fingerprint)
{
    if (scope.plate_id <= 0)
        throw std::invalid_argument("diagnostic plate_id must be positive");

    if (path.empty())
        throw std::invalid_argument("fingerprint path must not be empty");

    std::lock_guard<std::mutex> lock(m_mutex);
    m_plates[scope.plate_id][std::move(path)].push_back(Sample{scope, std::move(fingerprint)});
}

void Report::record_error(const Scope &scope, std::string path,
                          std::string message) noexcept
{
    try 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_errors.push_back(Error{scope, std::move(path), std::move(message)});
    } 
    catch (...) 
    {
        // Diagnostics must never change the slicing result. If even recording
        // the diagnostic error fails, there is no safe recovery path here.
    }
}

bool Report::save(const std::string &path, std::string &error) const
{
    try
    {
        std::map<int, FingerprintSamples> plates;
        std::vector<Error> errors;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            plates = m_plates;
            errors = m_errors;
        }

        json root = {{"schema_version", 1}, {"plates", json::array()}};
        for (const auto &[plate_id, samples_by_path] : plates)
        {
            json fingerprints = json::array();
            for (const auto &[fingerprint_path, samples] : samples_by_path)
            {
                json item = {
                    {"path", fingerprint_path},
                    {"fingerprint", aggregate(fingerprint_path, samples)},
                    {"sample_count", samples.size()}
                };
                json indexed_samples = serialize_indexed_samples(samples);
                if (!indexed_samples.empty())
                    item["samples"] = std::move(indexed_samples);
                fingerprints.push_back(std::move(item));
            }
            root["plates"].push_back({
                {"plate_id", plate_id},
                {"fingerprints", std::move(fingerprints)}
            });
        }

        if (!errors.empty())
        {
            std::sort(errors.begin(), errors.end(), error_less);
            root["errors"] = json::array();
            for (const Error &item : errors) 
            {
                json error_item = {
                    {"plate_id", item.scope.plate_id},
                    {"path", item.path},
                    {"message", item.message}
                };

                if (item.scope.object_id != no_object)
                    error_item["object_id"] = item.scope.object_id;

                if (item.scope.invocation_id != 0)
                    error_item["sample_id"] = item.scope.invocation_id - 1;

                root["errors"].push_back(std::move(error_item));
            }
        }

        boost::nowide::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream) 
        {
            error = "cannot open fingerprint report for writing: " + path;
            return false;
        }

        stream << root.dump(2) << '\n';
        if (!stream) 
        {
            error = "failed to write fingerprint report: " + path;
            return false;
        }
        return true;
    }
    catch (const std::exception &exception) 
    {
        error = "failed to build fingerprint report: " + std::string(exception.what());
        return false;
    }
}

} // namespace Slic3r::Diagnostics
