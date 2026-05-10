export module ResourceManager;

import std;
import UnorderedMap;
import <outcome/outcome.hpp>;

namespace fs = std::filesystem;
namespace outcome = OUTCOME_V2_NAMESPACE;
using OUTCOME_V2_NAMESPACE::failure;
using OUTCOME_V2_NAMESPACE::result;

export class Resource {
  public:
	virtual ~Resource() = default;
};

export class ResourceManager {
	std::mutex mutex;
	hive::unordered_map<std::string, std::shared_future<result<std::shared_ptr<Resource>, std::string>>> resources;

	template <typename T, typename... Args>
	result<std::shared_ptr<T>, std::string> load_impl(const std::string& key, Args&&... args) {
		std::unique_lock lock(mutex);

		if (const auto it = resources.find(key); it != resources.end()) {
			const auto fut = it->second;
			lock.unlock();
			auto result = fut.get();
			if (result) {
				return std::dynamic_pointer_cast<T>(result.value());
			}
			return failure(result.error());
		}

		// We are the first caller for this key. Insert a future so concurrent callers block
		std::promise<result<std::shared_ptr<Resource>, std::string>> promise;
		auto fut = promise.get_future().share();
		resources.emplace(key, fut);
		lock.unlock(); // Release before constructing. Constructor may call load() recursively

		try {
			auto res = std::make_shared<T>(std::forward<Args>(args)...);
			promise.set_value(res);
			return std::dynamic_pointer_cast<T>(res);
		} catch (const std::exception& e) {
			promise.set_value(failure(std::string(e.what())));
			return failure(std::string(e.what()));
		} catch (...) {
			const std::string err = "Unknown error loading resource";
			promise.set_value(failure(err));
			return failure(err);
		}
	}

  public:
	/// Loads and caches a resource until clear() is called.
	/// Thread-safe: concurrent loads of the same key construct the resource exactly once.
	template <typename T, typename... Args>
	result<std::shared_ptr<T>, std::string> load(const fs::path& path, const std::string& custom_identifier = "", Args... args) {
		static_assert(std::is_base_of_v<Resource, T>, "T must inherit from Resource");
		const std::string key = path.string() + T::name + custom_identifier;
		return load_impl<T>(key, path, std::forward<Args>(args)...);
	}

	template <typename T>
	result<std::shared_ptr<T>, std::string> load(const std::initializer_list<fs::path> paths) {
		static_assert(std::is_base_of_v<Resource, T>, "T must inherit from Resource");

		std::string key;
		for (const auto& path : paths) {
			key += path.string();
		}
		key += T::name;

		return load_impl<T>(key, paths);
	}

	/// Releases all cached resources. Call when unloading a map.
	void clear() {
		std::lock_guard lock(mutex);
		resources.clear();
	}
};

export extern ResourceManager resource_manager;
