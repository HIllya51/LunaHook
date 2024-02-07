#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <concrt.h>
#include <string>
#include <vector>
#include <deque>
#include <array>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <Psapi.h>
#include <functional>
#include <algorithm>
#include <regex>
#include <memory>
#include <optional>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <locale>
#include <cstdint>
#include <list>
#include <utility>
#include <cassert>
#include <variant>
#include"stringutils.h"
#ifdef _WIN64
constexpr bool x64 = true;
#else
constexpr bool x64 = false;
#endif

template <typename T, typename... Xs> struct ArrayImpl { using Type = std::tuple<T, Xs...>[]; };
template <typename T> struct ArrayImpl<T> { using Type = T[]; };
template <typename... Ts> using Array = typename ArrayImpl<Ts...>::Type;

template <auto F> using Functor = std::integral_constant<std::remove_reference_t<decltype(F)>, F>; // shouldn't need remove_reference_t but MSVC is bugged

struct PermissivePointer
{
	template <typename T> operator T*() { return (T*)p; }
	void* p;
};

template <typename HandleCloser = Functor<CloseHandle>>
class AutoHandle
{
public:
	AutoHandle(HANDLE h) : h(h) {}
	operator HANDLE() { return h.get(); }
	PHANDLE operator&() { static_assert(sizeof(*this) == sizeof(HANDLE)); assert(!h); return (PHANDLE)this; }
	operator bool() { return h.get() != NULL && h.get() != INVALID_HANDLE_VALUE; }

private:
	struct HandleCleaner { void operator()(void* h) { if (h != INVALID_HANDLE_VALUE) HandleCloser()(PermissivePointer{ h }); } };
	std::unique_ptr<void, HandleCleaner> h;
};

template<typename T, typename M = std::mutex>
class Synchronized
{
public:
	template <typename... Args>
	Synchronized(Args&&... args) : contents(std::forward<Args>(args)...) {}

	struct Locker
	{
		T* operator->() { return &contents; }
		std::unique_lock<M> lock;
		T& contents;
	};

	Locker Acquire() { return { std::unique_lock(m), contents }; }
	Locker operator->() { return Acquire(); }
	T Copy() { return Acquire().contents; }

private:
	T contents;
	M m;
};

template <typename F>
void SpawnThread(const F& f) // works in DllMain unlike std thread
{
	F* copy = new F(f);
	CloseHandle(CreateThread(nullptr, 0, [](void* copy)
	{
		(*(F*)copy)();
		delete (F*)copy;
		return 0UL;
	}, copy, 0, nullptr));
}

inline struct // should be inline but MSVC (linker) is bugged
{
	inline static BYTE DUMMY[100];
	template <typename T> operator T*() { static_assert(sizeof(T) < sizeof(DUMMY)); return (T*)DUMMY; }
} DUMMY;

inline auto Swallow = [](auto&&...) {};

template <typename T> std::optional<std::remove_cv_t<T>> Copy(T* ptr) { if (ptr) return *ptr; return {}; }



inline std::optional<std::wstring> GetModuleFilename(DWORD processId, HMODULE module = NULL)
{
	std::vector<wchar_t> buffer(MAX_PATH);
	if (AutoHandle<> process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId))
		if (GetModuleFileNameExW(process, module, buffer.data(), MAX_PATH)) return buffer.data();
	return {};
}

inline std::optional<std::wstring> GetModuleFilename(HMODULE module = NULL)
{
	std::vector<wchar_t> buffer(MAX_PATH);
	if (GetModuleFileNameW(module, buffer.data(), MAX_PATH)) return buffer.data();
	return {};
}
 