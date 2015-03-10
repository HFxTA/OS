/* Author: Artur Huletski (hatless.fox@gmail.com) */

#include <iostream>
#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
#include <cassert>

#pragma pack(push, 0)

struct Selector {
  union {
    uint16_t raw_data;
    struct {
      uint16_t rpl : 2;
      uint16_t is_ldt : 1;
      uint16_t index : 13;
    } l;
  } v;
};

static_assert(sizeof(Selector) == 2,
              "Wrong Selector size. Must be 16 bits");

struct Segment {
  union {
    uint64_t raw_data;
    struct {
      uint64_t limit_low : 16;
      uint64_t base_low : 24;
      uint64_t _unimportant2 : 7;
      uint64_t is_present : 1;
      uint64_t limit_hi : 4;
      uint64_t _unimportant1 : 3;
      uint64_t is_in_pages : 1; /* granularity bit */
      uint64_t base_hi : 8;
    } l;
  } v;
};

static_assert(sizeof(Segment) == 8,
	      "Wrong Segment size. Must be 64 bits");

struct LinearAddress {
  union {
    uint32_t raw_data;
    struct {
      uint32_t offset : 12;
      uint32_t table_i : 10;
      uint32_t dir_i : 10;
    } l;
  } v;
};

static_assert(sizeof(LinearAddress) == 4,
	      "Wrong Linear Address size. Must be 32 bits");

struct PageDirectory {
  union {
    uint32_t raw_data;
  } v;
};

static_assert(sizeof(PageDirectory) == 4,
	      "Wrong Page Directory size. Must be 32 bits");

struct PageEntry {
  union {
    uint32_t raw_data;
  } v;
};

static_assert(sizeof(PageEntry) == 4,
	      "Wrong Page Entry size. Must be 32 bits");

#pragma pack(pop)

struct MemorySystem {
  Selector selector;
  std::vector<Segment> gdt;
  std::vector<Segment> ldt;
  
  std::vector<PageDirectory> page_dir;
  std::vector<PageEntry> page_table;
};

template <typename T>
void initTable(std::vector<T>& table) {
  uint32_t entry_nm;
  std::cin >> std::dec >> entry_nm;
  while (entry_nm--) {
    T dt_entry;
    std::cin >> std::hex >> dt_entry.v.raw_data;
    table.push_back(dt_entry);
  }
}

uint32_t convertL2P(MemorySystem m, uint32_t laddr);

int main(int argc, char **argv) {
  MemorySystem memory;
  uint32_t laddr; /* logical address */
  std::string fail_msg;

  std::cin >> std::hex >> laddr;
  std::cin >> std::hex >> memory.selector.v.raw_data;
  initTable<Segment>(memory.gdt);
  initTable<Segment>(memory.ldt);
  initTable<PageDirectory>(memory.page_dir);
  initTable<PageEntry>(memory.page_table);

  try {
    std::cout << std::hex << convertL2P(memory, laddr) << std::endl;
    return 0;
  } catch(std::runtime_error& err) {
    std::cout << "INVALID" << std::endl;
     std::cout << "Reason: " << err.what() << std::endl;
    return -1;
  }
}

uint32_t convertL2P(MemorySystem m, uint32_t laddr) {
  // 1. Convert to Linear address
  // 1.1 Pick segment descriptor (ignoring RPL)
  if (m.selector.v.l.index == 0 && !m.selector.v.l.is_ldt) {
    throw std::runtime_error("Unable to get GDT[0] (null segment selector). p.3-8");
  }

  Segment segment = m.selector.v.l.is_ldt ?
    m.ldt[m.selector.v.l.index] : m.gdt[m.selector.v.l.index];
  if (!segment.v.l.is_present) {
    throw std::runtime_error("Segment is not present in memory. p.3-11");
  }

  uint64_t limit = segment.v.l.limit_hi << 4 | segment.v.l.limit_low;
  uint64_t seg_size = (limit + 1) * (segment.v.l.is_in_pages ? 2 << 12 : 1);

  if (seg_size <= laddr) {
    throw std::runtime_error("Offset is greater than seg. limit. #GP, p. 3-10");
  }

  uint64_t base_addr = segment.v.l.base_hi << 24 | segment.v.l.base_low;

  LinearAddress linaddr;
  linaddr.v.raw_data =  base_addr + laddr;
  assert(base_addr <= linaddr.v.raw_data && "Logical --> Linear Int overflow");

  // 2. Convert to Physical address

  // 2.1 Check table presence
  if (!(m.page_dir[linaddr.v.l.dir_i].v.raw_data & 1)) {
    throw std::runtime_error("Page table is not presented in memory");
  }

  //  std::cout << linaddr.v.l.dir_i << std::endl;
  //  assert(m.page_dir[linaddr.v.l.dir_i].v.raw_data & (1 << 7) == 0 && "Page Directory PS is 0");

  // we have only single page table (assignement conditions)...
  uint32_t page_data = m.page_table[linaddr.v.l.table_i].v.raw_data;

  if (!(page_data & 1)) {
    throw std::runtime_error("Page is not presented in memory");
  }

  uint32_t page_base = (page_data >> 12) << 12;
  return linaddr.v.l.offset + page_base;
}
