/* XMRig
 * Copyright (c) 2018-2025 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2025 XMRig       <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "backend/cpu/platform/BasicCpuInfo.h"
#include "base/tools/String.h"


#include <fstream>


#if __ARM_FEATURE_CRYPTO
#   include <sys/auxv.h>
#   if !defined(XMRIG_OS_FREEBSD)
#       include <asm/hwcap.h>
#   else
#       include <stdint.h>
#       include <machine/armreg.h>
#       ifndef ID_AA64ISAR0_AES_VAL
#           define ID_AA64ISAR0_AES_VAL ID_AA64ISAR0_AES
#       endif
#   endif
#endif


namespace xmrig {


extern String cpu_name_arm();

#ifdef XMRIG_RISCV
extern String cpu_name_riscv();
extern bool has_riscv_vector();
extern bool has_riscv_crypto();
#endif


} // namespace xmrig


void xmrig::BasicCpuInfo::init_arm()
{
#   ifdef XMRIG_RISCV
    // RISC-V specific detection
    auto name = cpu_name_riscv();
    if (!name.isNull()) {
        strncpy(m_brand, name, sizeof(m_brand) - 1);
    }
    
    // Check for crypto extensions (Zbk* extensions)
    m_flags.set(FLAG_AES, has_riscv_crypto());
    
    // RISC-V typically supports 1GB huge pages
    m_flags.set(FLAG_PDPE1GB, std::ifstream("/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages").good());
    
#   elif __ARM_FEATURE_CRYPTO
#   if defined(XMRIG_OS_FREEBSD)
    uint64_t isar0 = READ_SPECIALREG(id_aa64isar0_el1);
    m_flags.set(FLAG_AES, ID_AA64ISAR0_AES_VAL(isar0) >= ID_AA64ISAR0_AES_BASE);
#   else
    m_flags.set(FLAG_AES, getauxval(AT_HWCAP) & HWCAP_AES);
#   endif
#   endif

#   if defined(XMRIG_OS_UNIX) && !defined(XMRIG_RISCV)
    auto name = cpu_name_arm();
    if (!name.isNull()) {
        strncpy(m_brand, name, sizeof(m_brand) - 1);
    }

    m_flags.set(FLAG_PDPE1GB, std::ifstream("/sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages").good());
#   endif
}
