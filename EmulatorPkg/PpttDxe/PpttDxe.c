/** @file
  Install a minimal Processor Properties Topology Table (PPTT).

  The first PoC only publishes the ACPI table header. Processor and cache
  topology nodes will be added when the HOB contract is available.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi64.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/AcpiTable.h>

STATIC EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER  mPpttTable = {
  {
    SIGNATURE_32 ('P', 'P', 'T', 'T'),
    sizeof (EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER),
    EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION,
    0,
    { 0 },
    0,
    0,
    0,
    0
  }
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

  CopyMem (
    mPpttTable.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (mPpttTable.Header.OemId)
    );
  mPpttTable.Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  mPpttTable.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  mPpttTable.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  mPpttTable.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TableKey = 0;
  Status   = AcpiTable->InstallAcpiTable (
                          AcpiTable,
                          &mPpttTable,
                          sizeof (mPpttTable),
                          &TableKey
                          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "PpttDxe: installed minimal PPTT header\n"));
  return EFI_SUCCESS;
}
