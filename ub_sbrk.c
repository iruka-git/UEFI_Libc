#include <efi.h>
#include <efilib.h>
#include <efidef.h>

#include "ub_util.h"


//#include <Uefi.h>
//#include <Library/UefiBootServicesTableLib.h>

#define PAGE_SIZE 4096

//#define INITIALPAGES 4
#define INITIALPAGES 256

static VOID* heap_base = NULL;
static VOID* heap_current = NULL;
static VOID* heap_limit = NULL;

void* ub_sbrk(unsigned long increment)
{
    if (increment == 0) {
        return heap_current; // 現在のヒープ終端を返す
    }

    if (heap_base == NULL) {
        // 初回呼び出し時にヒープ領域を確保
        EFI_STATUS status;
        UINTN pages = INITIALPAGES; // 初期割り当てページ数（必要に応じて調整可能）
        status = gBS->AllocatePages(AllocateAnyPages, EfiBootServicesData, pages, (EFI_PHYSICAL_ADDRESS*)&heap_base);

        if (EFI_ERROR(status)) {
            return (VOID*)-1; // エラー時は (VOID*)-1 を返す
        }

        heap_current = heap_base;
        heap_limit = (VOID*)((UINT8*)heap_base + (pages * PAGE_SIZE));
    }

    // 必要な増加量を計算
    VOID* new_break = (VOID*)((UINT8*)heap_current + increment);

    // 増加が現在のヒープ範囲を超える場合は新たにメモリを確保
    if (new_break > heap_limit) {
        UINTN additional_pages = ((UINT8*)new_break - (UINT8*)heap_limit + PAGE_SIZE - 1) / PAGE_SIZE;
        EFI_STATUS status = gBS->AllocatePages(AllocateAnyPages, EfiBootServicesData, additional_pages, (EFI_PHYSICAL_ADDRESS*)&heap_limit);

        if (EFI_ERROR(status)) {
            return (VOID*)-1; // エラー時は (VOID*)-1 を返す
        }

        heap_limit = (VOID*)((UINT8*)heap_limit + (additional_pages * PAGE_SIZE));
    }

    // 増加分を反映
    VOID* old_break = heap_current;
    heap_current = new_break;

    return old_break;
}
