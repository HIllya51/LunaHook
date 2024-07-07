#include "EME.h"

/********************************************************************************************
EMEHook hook: (Contributed by Freaka)
  EmonEngine is used by LoveJuice company and TakeOut. Earlier builds were apparently
  called Runrun engine. String parsing varies a lot depending on the font settings and
  speed setting. E.g. without antialiasing (which very early versions did not have)
  uses TextOutA, fast speed triggers different functions then slow/normal. The user can
  set his own name and some odd control characters are used (0x09 for line break, 0x0D
  for paragraph end) which is parsed and put together on-the-fly while playing so script
  can't be read directly.
********************************************************************************************/
bool InsertEMEHook()
{
  ULONG addr = MemDbg::findCallAddress((ULONG)::IsDBCSLeadByte, processStartAddress, processStopAddress);
  // no needed as first call to IsDBCSLeadByte is correct, but sig could be used for further verification
  // WORD sig = 0x51C3;
  // while (c && (*(WORD*)(c-2)!=sig))
  //{
  //  //-0x1000 as FindCallOrJmpAbs always uses an offset of 0x1000
  //  c = Util::FindCallOrJmpAbs((DWORD)IsDBCSLeadByte,processStopAddress-c-0x1000+4,c-0x1000+4,false);
  //}
  if (!addr)
  {
    ConsoleOutput("EME: pattern does not exist");
    return false;
  }
  HookParam hp;
  hp.address = addr;
  hp.offset = get_reg(regs::eax);
  hp.type = NO_CONTEXT | DATA_INDIRECT | USING_STRING;
  ConsoleOutput("INSERT EmonEngine");

  // ConsoleOutput("EmonEngine, hook will only work with text speed set to slow or normal!");
  // else ConsoleOutput("Unknown EmonEngine engine");
  return NewHook(hp, "EmonEngine");
}
namespace
{
  
// LRU template class, recv two type params: key & value
template <typename Key>
class LRUCache
{

private:
   // cache capacity
   size_t _capacity = 0;
   // list _keys中key的指向位置
   std::unordered_map<Key, typename std::list<Key>::iterator> _cache;
   std::list<Key> _keys;

public:
   // construct function
   LRUCache(size_t size) : _capacity(size){};

   bool contains(Key key)
   {
      auto it = _cache.find(key);
      if (it == _cache.end())
      {
         return false;
      }; // 返回默认值
      _keys.splice(_keys.begin(), _keys, it->second);
      return true;
   }

   void put(Key key)
   {
      auto it = _cache.find(key);
      if (it != _cache.end())
      {
         _keys.splice(_keys.begin(), _keys, it->second);
         return;
      }

      if (_keys.size() == _capacity)
      {
         Key oldKey = _keys.back();
         _keys.pop_back();
         _cache.erase(oldKey);
      }

      _keys.push_front(key);
      _cache[key] = _keys.begin();
   }

};

  bool takeout()
  {
    //https://vndb.org/v6187
    //みちくさ～Loitering on the way～

    trigger_fun = [](LPVOID addr, hook_stack *stack)
    {
      if (addr != (LPVOID)GetGlyphOutlineA)
        return false;
      auto caller = stack->retaddr;
      auto add = MemDbg::findEnclosingAlignedFunction(caller);
      if (!add)
        return true;
      HookParam hp;
      hp.address = add;

      hp.type = USING_STRING;
      hp.offset = get_stack(4);
      hp.filter_fun = [](LPVOID data, size_t *size, HookParam *)
      {
        auto xx = std::string((char *)data, *size);
        static LRUCache<std::string> last(10);
        if (last.contains(xx))
          return false;
        last.put(xx);
        return true;
      };
      return NewHook(hp, "takeout");
    };
    return false;
  }
}
bool EME::attach_function()
{

  return InsertEMEHook() | takeout();
}