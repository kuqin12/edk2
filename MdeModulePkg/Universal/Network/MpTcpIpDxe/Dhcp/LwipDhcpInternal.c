/*
 * LwipDhcpInternal.c
 *
 *  Created on: Aug 29, 2018
 *      Author: mrabeda
 */

#include "LwipDhcpInternal.h"
#include <IndustryStandard/Dhcp.h>

//
// Value of DHCP overload option
//
#define DHCP_OVERLOAD_FILENAME    1
#define DHCP_OVERLOAD_SVRNAME     2
#define DHCP_OVERLOAD_BOTH        3

#define DHCP_OPTION_SWITCH        1
#define DHCP_OPTION_INT8          2
#define DHCP_OPTION_INT16         3
#define DHCP_OPTION_INT32         4
#define DHCP_OPTION_IP            5
#define DHCP_OPTION_IPPAIR        6

typedef struct {
  UINT8                     Index;
  UINT16                    Offset;
} DHCP_OPTION_COUNT;

typedef struct {
  DHCP_OPTION_COUNT         *OpCount;
  DHCP_OPTION               *Options;
  UINT8                     *Buf;
} DHCP_OPTION_CONTEXT;

typedef struct {
  UINT8                     Tag;
  INTN                      Type;
  INTN                      MinOccur;
  INTN                      MaxOccur;
  BOOLEAN                   Alert;
} DHCP_OPTION_FORMAT;

BOOLEAN
DhcpOptionIsValid (
  IN DHCP_OPTION_FORMAT     *Format,
  IN UINT8                  *OptValue,
  IN INTN                   Len
  );

EFI_STATUS
DhcpGetParameter (
  IN  UINT8                  Tag,
  IN  INTN                   Len,
  IN  UINT8                  *Data,
  OUT DHCP_PARAMETER         *Para
  );

EFI_STATUS
DhcpGetOptionLen (
  IN UINT8                  Tag,
  IN UINT8                  Len,
  IN UINT8                  *Data,
  IN VOID                   *Context
  );

EFI_STATUS
DhcpFillOption (
  IN UINT8                  Tag,
  IN UINT8                  Len,
  IN UINT8                  *Data,
  IN VOID                   *Context
  );

UINT8 *
DhcpAppendOption (
  OUT UINT8                  *Buf,
  IN  UINT8                  Tag,
  IN  UINT16                 DataLen,
  IN  UINT8                  *Data
  );

DHCP_OPTION_FORMAT *
DhcpFindOptionFormat (
  IN UINT8                  Tag
  );

DHCP_OPTION_FORMAT DhcpOptionFormats[] = {
  {DHCP4_TAG_NETMASK,        DHCP_OPTION_IP,     1, 1  , TRUE},
  {DHCP4_TAG_TIME_OFFSET,    DHCP_OPTION_INT32,  1, 1  , FALSE},
  {DHCP4_TAG_ROUTER,         DHCP_OPTION_IP,     1, -1 , TRUE},
  {DHCP4_TAG_TIME_SERVER,    DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_NAME_SERVER,    DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_DNS_SERVER,     DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_LOG_SERVER,     DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_COOKIE_SERVER,  DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_LPR_SERVER,     DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_IMPRESS_SERVER, DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_RL_SERVER,      DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_HOSTNAME,       DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_BOOTFILE_LEN,   DHCP_OPTION_INT16,  1, 1  , FALSE},
  {DHCP4_TAG_DUMP,           DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_DOMAINNAME,     DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_SWAP_SERVER,    DHCP_OPTION_IP,     1, 1  , FALSE},
  {DHCP4_TAG_ROOTPATH,       DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_EXTEND_PATH,    DHCP_OPTION_INT8,   1, -1 , FALSE},

  {DHCP4_TAG_IPFORWARD,      DHCP_OPTION_SWITCH, 1, 1  , FALSE},
  {DHCP4_TAG_NONLOCAL_SRR,   DHCP_OPTION_SWITCH, 1, 1  , FALSE},
  {DHCP4_TAG_POLICY_SRR,     DHCP_OPTION_IPPAIR, 1, -1 , FALSE},
  {DHCP4_TAG_EMTU,           DHCP_OPTION_INT16,  1, 1  , FALSE},
  {DHCP4_TAG_TTL,            DHCP_OPTION_INT8,   1, 1  , FALSE},
  {DHCP4_TAG_PATHMTU_AGE,    DHCP_OPTION_INT32,  1, 1  , FALSE},
  {DHCP4_TAG_PATHMTU_PLATEAU,DHCP_OPTION_INT16,  1, -1 , FALSE},

  {DHCP4_TAG_IFMTU,          DHCP_OPTION_INT16,  1, 1  , FALSE},
  {DHCP4_TAG_SUBNET_LOCAL,   DHCP_OPTION_SWITCH, 1, 1  , FALSE},
  {DHCP4_TAG_BROADCAST,      DHCP_OPTION_IP,     1, 1  , FALSE},
  {DHCP4_TAG_DISCOVER_MASK,  DHCP_OPTION_SWITCH, 1, 1  , FALSE},
  {DHCP4_TAG_SUPPLY_MASK,    DHCP_OPTION_SWITCH, 1, 1  , FALSE},
  {DHCP4_TAG_DISCOVER_ROUTE, DHCP_OPTION_SWITCH, 1, 1  , FALSE},
  {DHCP4_TAG_ROUTER_SOLICIT, DHCP_OPTION_IP,     1, 1  , FALSE},
  {DHCP4_TAG_STATIC_ROUTE,   DHCP_OPTION_IPPAIR, 1, -1 , FALSE},

  {DHCP4_TAG_TRAILER,        DHCP_OPTION_SWITCH, 1, 1  , FALSE},
  {DHCP4_TAG_ARPAGE,         DHCP_OPTION_INT32,  1, 1  , FALSE},
  {DHCP4_TAG_ETHER_ENCAP,    DHCP_OPTION_SWITCH, 1, 1  , FALSE},

  {DHCP4_TAG_TCP_TTL,        DHCP_OPTION_INT8,   1, 1  , FALSE},
  {DHCP4_TAG_KEEP_INTERVAL,  DHCP_OPTION_INT32,  1, 1  , FALSE},
  {DHCP4_TAG_KEEP_GARBAGE,   DHCP_OPTION_SWITCH, 1, 1  , FALSE},

  {DHCP4_TAG_NIS_DOMAIN,     DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_NIS_SERVER,     DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_NTP_SERVER,     DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_VENDOR,         DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_NBNS,           DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_NBDD,           DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_NBTYPE,         DHCP_OPTION_INT8,   1, 1  , FALSE},
  {DHCP4_TAG_NBSCOPE,        DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_XFONT,          DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_XDM,            DHCP_OPTION_IP,     1, -1 , FALSE},

  {DHCP4_TAG_REQUEST_IP,     DHCP_OPTION_IP,     1, 1  , FALSE},
  {DHCP4_TAG_LEASE,          DHCP_OPTION_INT32,  1, 1  , TRUE},
  {DHCP4_TAG_OVERLOAD,       DHCP_OPTION_INT8,   1, 1  , TRUE},
  {DHCP4_TAG_MSG_TYPE,       DHCP_OPTION_INT8,   1, 1  , TRUE},
  {DHCP4_TAG_SERVER_ID,      DHCP_OPTION_IP,     1, 1  , TRUE},
  {DHCP4_TAG_PARA_LIST,      DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_MESSAGE,        DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_MAXMSG,         DHCP_OPTION_INT16,  1, 1  , FALSE},
  {DHCP4_TAG_T1,             DHCP_OPTION_INT32,  1, 1  , TRUE},
  {DHCP4_TAG_T2,             DHCP_OPTION_INT32,  1, 1  , TRUE},
  {DHCP4_TAG_VENDOR_CLASS_ID,DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_CLIENT_ID,      DHCP_OPTION_INT8,   2, -1 , FALSE},

  {DHCP4_TAG_NISPLUS,        DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_NISPLUS_SERVER, DHCP_OPTION_IP,     1, -1 , FALSE},

  {DHCP4_TAG_TFTP,           DHCP_OPTION_INT8,   1, -1 , FALSE},
  {DHCP4_TAG_BOOTFILE,       DHCP_OPTION_INT8,   1, -1 , FALSE},

  {DHCP4_TAG_MOBILEIP,       DHCP_OPTION_IP,     0, -1 , FALSE},
  {DHCP4_TAG_SMTP,           DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_POP3,           DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_NNTP,           DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_WWW,            DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_FINGER,         DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_IRC,            DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_STTALK,         DHCP_OPTION_IP,     1, -1 , FALSE},
  {DHCP4_TAG_STDA,           DHCP_OPTION_IP,     1, -1 , FALSE},

  {DHCP4_TAG_CLASSLESS_ROUTE,DHCP_OPTION_INT8,   5, -1 , FALSE},
};

EFI_STATUS
DhcpParseOption (
  IN  EFI_DHCP4_PACKET      *Packet,
  OUT INTN                  *Count,
  OUT DHCP_OPTION           **OptionPoint
  )
{
  DHCP_OPTION_CONTEXT       Context;
  DHCP_OPTION               *Options;
  DHCP_OPTION_COUNT         *OptCount;
  EFI_STATUS                Status;
  UINT16                    TotalLen;
  INTN                      OptNum;
  INTN                      Index;

  ASSERT ((Count != NULL) && (OptionPoint != NULL));

  //
  // First compute how many options and how long each option is
  // with the "Key indexed counting" algorithms.
  //
  OptCount = AllocateZeroPool (DHCP_MAX_OPTIONS * sizeof (DHCP_OPTION_COUNT));

  if (OptCount == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = DhcpIterateOptions (Packet, DhcpGetOptionLen, OptCount);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Before the loop, Offset is the length of the option. After loop,
  // OptCount[Index].Offset specifies the offset into the continuous
  // option value buffer to put the data.
  //
  TotalLen  = 0;
  OptNum    = 0;

  for (Index = 0; Index < DHCP_MAX_OPTIONS; Index++) {
    if (OptCount[Index].Offset != 0) {
      OptCount[Index].Index   = (UINT8) OptNum;

      TotalLen                = (UINT16) (TotalLen + OptCount[Index].Offset);
      OptCount[Index].Offset  = (UINT16) (TotalLen - OptCount[Index].Offset);

      OptNum++;
    }
  }

  *Count        = OptNum;
  *OptionPoint  = NULL;

  if (OptNum == 0) {
    goto ON_EXIT;
  }

  //
  // Allocate a buffer to hold the DHCP options, and after that, a
  // continuous buffer to put all the options' data.
  //
  Options = AllocateZeroPool ((UINTN) (OptNum * sizeof (DHCP_OPTION)) + TotalLen);

  if (Options == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Context.OpCount = OptCount;
  Context.Options = Options;
  Context.Buf     = (UINT8 *) (Options + OptNum);

  Status          = DhcpIterateOptions (Packet, DhcpFillOption, &Context);

  if (EFI_ERROR (Status)) {
    FreePool (Options);
    goto ON_EXIT;
  }

  *OptionPoint = Options;

ON_EXIT:
  FreePool (OptCount);
  return Status;
}

EFI_STATUS
DhcpBuild (
  IN  EFI_DHCP4_PACKET        *SeedPacket,
  IN  UINT32                  DeleteCount,
  IN  UINT8                   *DeleteList     OPTIONAL,
  IN  UINT32                  AppendCount,
  IN  EFI_DHCP4_PACKET_OPTION *AppendList[]   OPTIONAL,
  OUT EFI_DHCP4_PACKET        **NewPacket
  )
{
  DHCP_OPTION               *Mark;
  DHCP_OPTION               *SeedOptions;
  EFI_DHCP4_PACKET          *Packet;
  EFI_STATUS                Status;
  INTN                      Count;
  UINT32                    Index;
  UINT32                    Len;
  UINT8                     *Buf;

  //
  // Use an array of DHCP_OPTION to mark the existance
  // and position of each valid options.
  //
  Mark = AllocatePool (sizeof (DHCP_OPTION) * DHCP_MAX_OPTIONS);

  if (Mark == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < DHCP_MAX_OPTIONS; Index++) {
    Mark[Index].Tag = (UINT8) Index;
    Mark[Index].Len = 0;
  }

  //
  // Get list of the options from the seed packet, then put
  // them to the mark array according to their tags.
  //
  SeedOptions = NULL;
  Status      = DhcpParseOption (SeedPacket, &Count, &SeedOptions);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  if (SeedOptions != NULL) {
    for (Index = 0; Index < (UINT32) Count; Index++) {
      Mark[SeedOptions[Index].Tag] = SeedOptions[Index];
    }
  }

  //
  // Mark the option's length is zero if it is in the DeleteList.
  //
  for (Index = 0; Index < DeleteCount; Index++) {
    Mark[DeleteList[Index]].Len = 0;
  }

  //
  // Add or replace the option if it is in the append list.
  //
  for (Index = 0; Index < AppendCount; Index++) {
    Mark[AppendList[Index]->OpCode].Len  = AppendList[Index]->Length;
    Mark[AppendList[Index]->OpCode].Data = AppendList[Index]->Data;
  }

  //
  // compute the new packet length. No need to add 1 byte for
  // EOP option since EFI_DHCP4_PACKET includes one extra byte
  // for option. It is necessary to split the option if it is
  // longer than 255 bytes.
  //
  Len = sizeof (EFI_DHCP4_PACKET);

  for (Index = 0; Index < DHCP_MAX_OPTIONS; Index++) {
    if (Mark[Index].Len != 0) {
      Len += ((Mark[Index].Len + 254) / 255) * 2 + Mark[Index].Len;
    }
  }

  Status  = EFI_OUT_OF_RESOURCES;
  Packet  = (EFI_DHCP4_PACKET *) AllocatePool (Len);

  if (Packet == NULL) {
    goto ON_ERROR;
  }

  Packet->Size         = Len;
  Packet->Length       = 0;
  CopyMem (&Packet->Dhcp4.Header, &SeedPacket->Dhcp4.Header, sizeof (Packet->Dhcp4.Header));
  Packet->Dhcp4.Magik  = DHCP_OPTION_MAGIC;
  Buf                  = Packet->Dhcp4.Option;

  for (Index = 0; Index < DHCP_MAX_OPTIONS; Index++) {
    if (Mark[Index].Len != 0) {
      Buf = DhcpAppendOption (Buf, Mark[Index].Tag, Mark[Index].Len, Mark[Index].Data);
    }
  }

  *(Buf++)        = DHCP4_TAG_EOP;
  Packet->Length  = sizeof (EFI_DHCP4_HEADER) + sizeof (UINT32)
                      + (UINT32) (Buf - Packet->Dhcp4.Option);

  *NewPacket      = Packet;
  Status          = EFI_SUCCESS;

ON_ERROR:
  if (SeedOptions != NULL) {
    FreePool (SeedOptions);
  }

  FreePool (Mark);
  return Status;
}

EFI_STATUS
DhcpIterateBufferOptions (
  IN  UINT8                 *Buffer,
  IN  INTN                  BufLen,
  IN  DHCP_CHECK_OPTION     Check             OPTIONAL,
  IN  VOID                  *Context,
  OUT UINT8                 *Overload         OPTIONAL
  )
{
  INTN                      Cur;
  UINT8                     Tag;
  UINT8                     Len;

  Cur = 0;

  while (Cur < BufLen) {
    Tag = Buffer[Cur];

    if (Tag == DHCP4_TAG_PAD) {
      Cur++;
      continue;
    } else if (Tag == DHCP4_TAG_EOP) {
      return EFI_SUCCESS;
    }

    Cur++;

    if (Cur == BufLen) {
      return EFI_INVALID_PARAMETER;
    }

    Len = Buffer[Cur++];

    if (Cur + Len > BufLen) {
      return EFI_INVALID_PARAMETER;
    }

    if ((Tag == DHCP4_TAG_OVERLOAD) && (Overload != NULL)) {
      if (Len != 1) {
        return EFI_INVALID_PARAMETER;
      }

      *Overload = Buffer[Cur];
    }

    if ((Check != NULL) && EFI_ERROR (Check (Tag, Len, Buffer + Cur, Context))) {
      return EFI_INVALID_PARAMETER;
    }

    Cur += Len;
  }

  //
  // Each option buffer is expected to end with an EOP
  //
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
DhcpIterateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  IN  DHCP_CHECK_OPTION     Check         OPTIONAL,
  IN  VOID                  *Context
  )
{
  EFI_STATUS                Status;
  UINT8                     Overload;

  Overload = 0;

  Status   = DhcpIterateBufferOptions (
               Packet->Dhcp4.Option,
               Packet->Length - sizeof (EFI_DHCP4_HEADER) - sizeof (UINT32),
               Check,
               Context,
               &Overload
               );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Overload == DHCP_OVERLOAD_FILENAME) || (Overload == DHCP_OVERLOAD_BOTH)) {
    Status = DhcpIterateBufferOptions (
               (UINT8 *) Packet->Dhcp4.Header.BootFileName,
               128,
               Check,
               Context,
               NULL
               );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if ((Overload == DHCP_OVERLOAD_SVRNAME) || (Overload == DHCP_OVERLOAD_BOTH)) {
    Status = DhcpIterateBufferOptions (
               (UINT8 *) Packet->Dhcp4.Header.ServerName,
               64,
               Check,
               Context,
               NULL
               );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DhcpGetOptionLen (
  IN UINT8                  Tag,
  IN UINT8                  Len,
  IN UINT8                  *Data,
  IN VOID                   *Context
  )
{
  DHCP_OPTION_COUNT         *OpCount;

  OpCount             = (DHCP_OPTION_COUNT *) Context;
  OpCount[Tag].Offset = (UINT16) (OpCount[Tag].Offset + Len);

  return EFI_SUCCESS;
}

EFI_STATUS
DhcpFillOption (
  IN UINT8                  Tag,
  IN UINT8                  Len,
  IN UINT8                  *Data,
  IN VOID                   *Context
  )
{
  DHCP_OPTION_CONTEXT       *OptContext;
  DHCP_OPTION_COUNT         *OptCount;
  DHCP_OPTION               *Options;
  UINT8                     *Buf;
  UINT8                     Index;

  OptContext  = (DHCP_OPTION_CONTEXT *) Context;

  OptCount    = OptContext->OpCount;
  Index       = OptCount[Tag].Index;
  Options     = OptContext->Options;
  Buf         = OptContext->Buf;

  if (Options[Index].Data == NULL) {
    Options[Index].Tag  = Tag;
    Options[Index].Data = Buf + OptCount[Tag].Offset;
  }

  CopyMem (Buf + OptCount[Tag].Offset, Data, Len);

  OptCount[Tag].Offset  = (UINT16) (OptCount[Tag].Offset + Len);
  Options[Index].Len    = (UINT16) (Options[Index].Len + Len);
  return EFI_SUCCESS;
}

UINT8 *
DhcpAppendOption (
  OUT UINT8                  *Buf,
  IN  UINT8                  Tag,
  IN  UINT16                 DataLen,
  IN  UINT8                  *Data
  )
{
  INTN                      Index;
  INTN                      Len;

  ASSERT (DataLen != 0);

  for (Index = 0; Index < (DataLen + 254) / 255; Index++) {
    Len      = MIN (255, DataLen - Index * 255);

    *(Buf++) = Tag;
    *(Buf++) = (UINT8) Len;
    CopyMem (Buf, Data + Index * 255, (UINTN) Len);

    Buf     += Len;
  }

  return Buf;
}

EFI_STATUS
DhcpValidateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  OUT DHCP_PARAMETER        **Para       OPTIONAL
  )
{
  DHCP_PARAMETER            Parameter;
  DHCP_OPTION_FORMAT        *Format;
  DHCP_OPTION               *AllOption;
  DHCP_OPTION               *Option;
  EFI_STATUS                Status;
  BOOLEAN                   Updated;
  INTN                      Count;
  INTN                      Index;

  if (Para != NULL) {
    *Para = NULL;
  }

  AllOption = NULL;

  Status = DhcpParseOption (Packet, &Count, &AllOption);
  if (EFI_ERROR (Status) || (Count == 0)) {
    return Status;
  }
  ASSERT (AllOption != NULL);

  Updated = FALSE;
  ZeroMem (&Parameter, sizeof (Parameter));

  for (Index = 0; Index < Count; Index++) {
    Option = &AllOption[Index];

    //
    // Find the format of the option then validate it.
    //
    Format = DhcpFindOptionFormat (Option->Tag);

    if (Format == NULL) {
      continue;
    }

    if (!DhcpOptionIsValid (Format, Option->Data, Option->Len)) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    //
    // Get the client interested parameters
    //
    if (Format->Alert && (Para != NULL)) {
      Updated = TRUE;
      Status  = DhcpGetParameter (Option->Tag, Option->Len, Option->Data, &Parameter);

      if (EFI_ERROR (Status)) {
        goto ON_EXIT;
      }
    }
  }

  if (Updated && (Para != NULL)) {
    *Para = AllocateCopyPool (sizeof (DHCP_PARAMETER), &Parameter);
    if (*Para == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }
  }

ON_EXIT:
  FreePool (AllOption);
  return Status;
}

DHCP_OPTION_FORMAT *
DhcpFindOptionFormat (
  IN UINT8                  Tag
  )
{
  INTN                      Left;
  INTN                      Right;
  INTN                      Middle;

  Left  = 0;
  Right = sizeof (DhcpOptionFormats) / sizeof (DHCP_OPTION_FORMAT) - 1;

  while (Right >= Left) {
    Middle = (Left + Right) / 2;

    if (Tag == DhcpOptionFormats[Middle].Tag) {
      return &DhcpOptionFormats[Middle];
    }

    if (Tag < DhcpOptionFormats[Middle].Tag) {
      Right = Middle - 1;
    } else {
      Left  = Middle + 1;
    }
  }

  return NULL;
}

BOOLEAN
DhcpOptionIsValid (
  IN DHCP_OPTION_FORMAT     *Format,
  IN UINT8                  *OptValue,
  IN INTN                   Len
  )
{
  INTN                      Unit;
  INTN                      Occur;
  INTN                      Index;

  Unit = 0;

  switch (Format->Type) {
  case DHCP_OPTION_SWITCH:
  case DHCP_OPTION_INT8:
    Unit = 1;
    break;

  case DHCP_OPTION_INT16:
    Unit = 2;
    break;

  case DHCP_OPTION_INT32:
  case DHCP_OPTION_IP:
    Unit = 4;
    break;

  case DHCP_OPTION_IPPAIR:
    Unit = 8;
    break;
  }

  ASSERT (Unit != 0);

  //
  // Validate that the option appears in the full units.
  //
  if ((Len % Unit) != 0) {
    return FALSE;
  }

  //
  // Validate the occurance of the option unit is with in [MinOccur, MaxOccur]
  //
  Occur = Len / Unit;

  if (((Format->MinOccur != -1) && (Occur < Format->MinOccur)) ||
      ((Format->MaxOccur != -1) && (Occur > Format->MaxOccur))
      ) {
    return FALSE;
  }

  //
  // If the option is of type switch, only 0/1 are valid values.
  //
  if (Format->Type == DHCP_OPTION_SWITCH) {
    for (Index = 0; Index < Occur; Index++) {
      if ((OptValue[Index] != 0) && (OptValue[Index] != 1)) {
        return FALSE;
      }
    }
  }

  return TRUE;
}

EFI_STATUS
DhcpGetParameter (
  IN  UINT8                  Tag,
  IN  INTN                   Len,
  IN  UINT8                  *Data,
  OUT DHCP_PARAMETER         *Para
  )
{
  switch (Tag) {
  case DHCP4_TAG_NETMASK:
    Para->NetMask = NetGetUint32 (Data);
    break;

  case DHCP4_TAG_ROUTER:
    //
    // Return the first router to consumer which is the preferred one
    //
    Para->Router = NetGetUint32 (Data);
    break;

  case DHCP4_TAG_LEASE:
    Para->Lease = NetGetUint32 (Data);
    break;

  case DHCP4_TAG_OVERLOAD:
    Para->Overload = *Data;

    if ((Para->Overload < 1) || (Para->Overload > 3)) {
      return EFI_INVALID_PARAMETER;
    }
    break;

  case DHCP4_TAG_MSG_TYPE:
    Para->DhcpType = *Data;

    if ((Para->DhcpType < 1) || (Para->DhcpType > 9)) {
      return EFI_INVALID_PARAMETER;
    }
    break;

  case DHCP4_TAG_SERVER_ID:
    Para->ServerId = NetGetUint32 (Data);
    break;

  case DHCP4_TAG_T1:
    Para->T1 = NetGetUint32 (Data);
    break;

  case DHCP4_TAG_T2:
    Para->T2 = NetGetUint32 (Data);
    break;
  }

  return EFI_SUCCESS;
}

//
// Note to self: this function is only called by LWIP when DHCP event occurs.
//
err_t
DhcpStatusCallback (
  IN  struct netif          *NetIf,
  IN  dhcp_event            DhcpEvent,
  IN  struct pbuf           *Packet,
  IN  struct pbuf           **NewPacket
  )
{
  LWIP_NET_DEVICE       *Device;
  DBG ("DhcpStatusCallback: Called with event: %d\n", DhcpEvent);

  Device = LWIP_DEVICE_FROM_NETIF (NetIf);

  ASSERT (Device != NULL);
  ASSERT (Device->DhcpCallbackEvent != NULL);

  //
  // This function is called by LWIP thread. Using event mechanism to signal
  // different core using DHCP service.
  //
  Device->DhcpCallbackSem = TRUE;
  gBS->SignalEvent (Device->DhcpCallbackEvent);

  //
  // Wait for event completion.
  //
  while (Device->DhcpCallbackSem) {
    CpuPause ();
  }
  DBG ("DhcpStatusCallback: Waiter notified.\n");

  return ERR_OK;
}

VOID
DhcpNotifyUser (
  IN  EFI_EVENT             Event,
  IN  VOID                  *Context
  )
{
  LWIP_NET_DEVICE       *Device;

  DBG ("DhcpNotifyUser: Called\n");

  Device = (LWIP_NET_DEVICE*) Context;
  ASSERT (Device != NULL);
  NET_CHECK_SIGNATURE (Device, LWIP_NET_DEVICE_SIGNATURE);

  Device->DhcpCallbackSem = FALSE;
}
