/** @file
  Compile-time Processor Properties Topology Table for the Rhea1 PoC.

  The table is constant: it does not consume PlatformInfo, HOBs, PCD updates,
  or runtime topology information. The MADT remains the source of processor
  discovery and enablement; this PPTT describes the maximum two-socket topology.

  Cache geometry is based on the supplied SiPearl/Ampere PPTT reference. The
  Rhea1 SLC size is fixed at 80 MiB per socket.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi64.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/AcpiTable.h>

#define RHEA1_PPTT_SOCKET_COUNT       2
#define RHEA1_PPTT_CORES_PER_SOCKET  128
#define RHEA1_PPTT_CORE_COUNT        (RHEA1_PPTT_SOCKET_COUNT * RHEA1_PPTT_CORES_PER_SOCKET)

#define RHEA1_PPTT_L1I_SIZE  (64U * 1024U)
#define RHEA1_PPTT_L1D_SIZE  (64U * 1024U)
#define RHEA1_PPTT_L2_SIZE   (1U * 1024U * 1024U)
#define RHEA1_PPTT_SLC_SIZE  (80U * 1024U * 1024U)
#define RHEA1_PPTT_LINE_SIZE 64U
#define RHEA1_PPTT_L1_SETS  0x100U
#define RHEA1_PPTT_L1_ASSOC 4U
#define RHEA1_PPTT_L2_SETS  0x800U
#define RHEA1_PPTT_L2_ASSOC 8U

#define RHEA1_PPTT_ACPI_PROCESSOR_ID(Socket, Core)  (((Socket) << 24) | ((Core) << 16))
#define RHEA1_PPTT_CORE_INDEX(Socket, Core)         ((Socket) * RHEA1_PPTT_CORES_PER_SOCKET + (Core))
#define RHEA1_PPTT_ARRAY_OFFSET(Field, Index) \
  ((UINT32)(OFFSET_OF (RHEA1_PPTT_TABLE, Field) + \
            (sizeof (((RHEA1_PPTT_TABLE *)0)->Field[0]) * (Index))))

typedef struct {
  EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR  Processor;
  UINT32                                  Resources[1];
} RHEA1_PPTT_SOCKET;

typedef struct {
  EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR  Processor;
  UINT32                                  Resources[3];
} RHEA1_PPTT_CORE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER              Header;
  EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE        Slc[RHEA1_PPTT_SOCKET_COUNT];
  EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE        L2[RHEA1_PPTT_CORE_COUNT];
  EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE        L1D[RHEA1_PPTT_CORE_COUNT];
  EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE        L1I[RHEA1_PPTT_CORE_COUNT];
  RHEA1_PPTT_SOCKET                         Socket[RHEA1_PPTT_SOCKET_COUNT];
  RHEA1_PPTT_CORE                           Core[RHEA1_PPTT_CORE_COUNT];
} RHEA1_PPTT_TABLE;

#define RHEA1_PPTT_CACHE_FLAGS \
  { .SizePropertyValid = 1, .NumberOfSetsValid = 1, .AssociativityValid = 1, \
    .AllocationTypeValid = 1, .CacheTypeValid = 1, .WritePolicyValid = 1, \
    .LineSizeValid = 1, .CacheIdValid = 1 }

#define RHEA1_PPTT_CACHE_ATTRIBUTES(Type) \
  { .AllocationType = EFI_ACPI_6_4_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE, \
    .CacheType = (Type), .WritePolicy = EFI_ACPI_6_4_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK, .Reserved = 0 }

#define RHEA1_PPTT_SLC(SocketId) \
  { .Type = EFI_ACPI_6_4_PPTT_TYPE_CACHE, .Length = sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE), \
    .Reserved = { 0, 0 }, .Flags = RHEA1_PPTT_CACHE_FLAGS, .NextLevelOfCache = 0, \
    .Size = RHEA1_PPTT_SLC_SIZE, .NumberOfSets = 1, .Associativity = 1, \
    .Attributes = RHEA1_PPTT_CACHE_ATTRIBUTES (EFI_ACPI_6_4_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED), \
    .LineSize = RHEA1_PPTT_LINE_SIZE, .CacheId = 0x30 }

#define RHEA1_PPTT_L2(SocketId, CoreId) \
  { .Type = EFI_ACPI_6_4_PPTT_TYPE_CACHE, .Length = sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE), \
    .Reserved = { 0, 0 }, .Flags = RHEA1_PPTT_CACHE_FLAGS, \
    .NextLevelOfCache = RHEA1_PPTT_ARRAY_OFFSET (Slc, SocketId), .Size = RHEA1_PPTT_L2_SIZE, \
    .NumberOfSets = RHEA1_PPTT_L2_SETS, .Associativity = RHEA1_PPTT_L2_ASSOC, \
    .Attributes = RHEA1_PPTT_CACHE_ATTRIBUTES (EFI_ACPI_6_4_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED), \
    .LineSize = RHEA1_PPTT_LINE_SIZE, .CacheId = 0x20 }

#define RHEA1_PPTT_L1D(SocketId, CoreId) \
  { .Type = EFI_ACPI_6_4_PPTT_TYPE_CACHE, .Length = sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE), \
    .Reserved = { 0, 0 }, .Flags = RHEA1_PPTT_CACHE_FLAGS, \
    .NextLevelOfCache = RHEA1_PPTT_ARRAY_OFFSET (L2, RHEA1_PPTT_CORE_INDEX (SocketId, CoreId)), \
    .Size = RHEA1_PPTT_L1D_SIZE, .NumberOfSets = RHEA1_PPTT_L1_SETS, .Associativity = RHEA1_PPTT_L1_ASSOC, \
    .Attributes = RHEA1_PPTT_CACHE_ATTRIBUTES (EFI_ACPI_6_4_CACHE_ATTRIBUTES_CACHE_TYPE_DATA), \
    .LineSize = RHEA1_PPTT_LINE_SIZE, .CacheId = 0x10 }

#define RHEA1_PPTT_L1I(SocketId, CoreId) \
  { .Type = EFI_ACPI_6_4_PPTT_TYPE_CACHE, .Length = sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE), \
    .Reserved = { 0, 0 }, .Flags = RHEA1_PPTT_CACHE_FLAGS, \
    .NextLevelOfCache = RHEA1_PPTT_ARRAY_OFFSET (L2, RHEA1_PPTT_CORE_INDEX (SocketId, CoreId)), \
    .Size = RHEA1_PPTT_L1I_SIZE, .NumberOfSets = RHEA1_PPTT_L1_SETS, .Associativity = RHEA1_PPTT_L1_ASSOC, \
    .Attributes = RHEA1_PPTT_CACHE_ATTRIBUTES (EFI_ACPI_6_4_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION), \
    .LineSize = RHEA1_PPTT_LINE_SIZE, .CacheId = 0x11 }

#define RHEA1_PPTT_PROCESSOR_FLAGS(Package, IdValid, Leaf) \
  { .PhysicalPackage = (Package), .AcpiProcessorIdValid = (IdValid), .ProcessorIsAThread = 0, \
    .NodeIsALeaf = (Leaf), .IdenticalImplementation = 1 }

#define RHEA1_PPTT_SOCKET_NODE(SocketId) \
  { .Processor = { .Type = EFI_ACPI_6_4_PPTT_TYPE_PROCESSOR, \
  .Length = sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR) + sizeof (UINT32), .Reserved = { 0, 0 }, \
      .Flags = RHEA1_PPTT_PROCESSOR_FLAGS (1, 0, 0), .Parent = 0, .AcpiProcessorId = 0, \
      .NumberOfPrivateResources = 1 }, \
    .Resources = { RHEA1_PPTT_ARRAY_OFFSET (Slc, SocketId) } }

#define RHEA1_PPTT_CORE_NODE(SocketId, CoreId) \
  { .Processor = { .Type = EFI_ACPI_6_4_PPTT_TYPE_PROCESSOR, \
  .Length = sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR) + (3 * sizeof (UINT32)), .Reserved = { 0, 0 }, \
      .Flags = RHEA1_PPTT_PROCESSOR_FLAGS (0, 1, 1), \
      .Parent = RHEA1_PPTT_ARRAY_OFFSET (Socket, SocketId), \
      .AcpiProcessorId = RHEA1_PPTT_ACPI_PROCESSOR_ID (SocketId, CoreId), \
      .NumberOfPrivateResources = 2 }, \
    .Resources = { \
      RHEA1_PPTT_ARRAY_OFFSET (L1I, RHEA1_PPTT_CORE_INDEX (SocketId, CoreId)), \
      RHEA1_PPTT_ARRAY_OFFSET (L1D, RHEA1_PPTT_CORE_INDEX (SocketId, CoreId)) } }

#define RHEA1_PPTT_128(Macro, SocketId) \
  Macro(SocketId,0),Macro(SocketId,1),Macro(SocketId,2),Macro(SocketId,3),Macro(SocketId,4),Macro(SocketId,5),Macro(SocketId,6),Macro(SocketId,7), \
  Macro(SocketId,8),Macro(SocketId,9),Macro(SocketId,10),Macro(SocketId,11),Macro(SocketId,12),Macro(SocketId,13),Macro(SocketId,14),Macro(SocketId,15), \
  Macro(SocketId,16),Macro(SocketId,17),Macro(SocketId,18),Macro(SocketId,19),Macro(SocketId,20),Macro(SocketId,21),Macro(SocketId,22),Macro(SocketId,23), \
  Macro(SocketId,24),Macro(SocketId,25),Macro(SocketId,26),Macro(SocketId,27),Macro(SocketId,28),Macro(SocketId,29),Macro(SocketId,30),Macro(SocketId,31), \
  Macro(SocketId,32),Macro(SocketId,33),Macro(SocketId,34),Macro(SocketId,35),Macro(SocketId,36),Macro(SocketId,37),Macro(SocketId,38),Macro(SocketId,39), \
  Macro(SocketId,40),Macro(SocketId,41),Macro(SocketId,42),Macro(SocketId,43),Macro(SocketId,44),Macro(SocketId,45),Macro(SocketId,46),Macro(SocketId,47), \
  Macro(SocketId,48),Macro(SocketId,49),Macro(SocketId,50),Macro(SocketId,51),Macro(SocketId,52),Macro(SocketId,53),Macro(SocketId,54),Macro(SocketId,55), \
  Macro(SocketId,56),Macro(SocketId,57),Macro(SocketId,58),Macro(SocketId,59),Macro(SocketId,60),Macro(SocketId,61),Macro(SocketId,62),Macro(SocketId,63), \
  Macro(SocketId,64),Macro(SocketId,65),Macro(SocketId,66),Macro(SocketId,67),Macro(SocketId,68),Macro(SocketId,69),Macro(SocketId,70),Macro(SocketId,71), \
  Macro(SocketId,72),Macro(SocketId,73),Macro(SocketId,74),Macro(SocketId,75),Macro(SocketId,76),Macro(SocketId,77),Macro(SocketId,78),Macro(SocketId,79), \
  Macro(SocketId,80),Macro(SocketId,81),Macro(SocketId,82),Macro(SocketId,83),Macro(SocketId,84),Macro(SocketId,85),Macro(SocketId,86),Macro(SocketId,87), \
  Macro(SocketId,88),Macro(SocketId,89),Macro(SocketId,90),Macro(SocketId,91),Macro(SocketId,92),Macro(SocketId,93),Macro(SocketId,94),Macro(SocketId,95), \
  Macro(SocketId,96),Macro(SocketId,97),Macro(SocketId,98),Macro(SocketId,99),Macro(SocketId,100),Macro(SocketId,101),Macro(SocketId,102),Macro(SocketId,103), \
  Macro(SocketId,104),Macro(SocketId,105),Macro(SocketId,106),Macro(SocketId,107),Macro(SocketId,108),Macro(SocketId,109),Macro(SocketId,110),Macro(SocketId,111), \
  Macro(SocketId,112),Macro(SocketId,113),Macro(SocketId,114),Macro(SocketId,115),Macro(SocketId,116),Macro(SocketId,117),Macro(SocketId,118),Macro(SocketId,119), \
  Macro(SocketId,120),Macro(SocketId,121),Macro(SocketId,122),Macro(SocketId,123),Macro(SocketId,124),Macro(SocketId,125),Macro(SocketId,126),Macro(SocketId,127)

STATIC CONST RHEA1_PPTT_TABLE  mPpttTable = {
  .Header = {
    .Signature = SIGNATURE_32 ('P', 'P', 'T', 'T'),
    .Length = sizeof (RHEA1_PPTT_TABLE),
    .Revision = EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION,
    .Checksum = 0,
    .OemId = { 'I', 'N', 'T', 'E', 'L', ' ' },
    .OemTableId = SIGNATURE_64 ('E', 'D', 'K', '2', ' ', ' ', ' ', ' '),
    .OemRevision = 2,
    .CreatorId = SIGNATURE_32 (' ', ' ', ' ', ' '),
    .CreatorRevision = 0x01000013
  },
  .Slc = { RHEA1_PPTT_SLC (0), RHEA1_PPTT_SLC (1) },
  .L2 = { RHEA1_PPTT_128 (RHEA1_PPTT_L2, 0), RHEA1_PPTT_128 (RHEA1_PPTT_L2, 1) },
  .L1D = { RHEA1_PPTT_128 (RHEA1_PPTT_L1D, 0), RHEA1_PPTT_128 (RHEA1_PPTT_L1D, 1) },
  .L1I = { RHEA1_PPTT_128 (RHEA1_PPTT_L1I, 0), RHEA1_PPTT_128 (RHEA1_PPTT_L1I, 1) },
  .Socket = { RHEA1_PPTT_SOCKET_NODE (0), RHEA1_PPTT_SOCKET_NODE (1) },
  .Core = { RHEA1_PPTT_128 (RHEA1_PPTT_CORE_NODE, 0), RHEA1_PPTT_128 (RHEA1_PPTT_CORE_NODE, 1) }
};

EFI_STATUS
EFIAPI
PpttDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  EFI_STATUS               Status;
  UINTN                    TableKey;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TableKey = 0;
  Status = AcpiTable->InstallAcpiTable (AcpiTable, (VOID *)&mPpttTable, sizeof (mPpttTable), &TableKey);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "PpttDxe: installed constant Rhea1 PPTT\n"));
  return EFI_SUCCESS;
}
