/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:

	Abstract:

	Revision History:
	Who 		When			What
	--------	----------		----------------------------------------------
*/

#include "rt_config.h"

#ifdef SCAN_SUPPORT
static INT scan_ch_restore(RTMP_ADAPTER *pAd, UCHAR OpMode)
{
	INT bw, ch;
#ifdef APCLI_SUPPORT
#ifdef APCLI_CERT_SUPPORT
	UCHAR  ScanType = pAd->MlmeAux.ScanType;
#endif /*APCLI_CERT_SUPPORT*/
#endif /*APCLI_SUPPORT*/

#ifdef WH_EZ_SETUP
	if (IS_ADPTR_EZ_SETUP_ENABLED(pAd) && (pAd->ApCfg.ScanReqwdev != NULL)
		&& ez_handle_scan_channel_restore(pAd->ApCfg.ScanReqwdev)) {

	}
#endif /* WH_EZ_SETUP */
#ifdef DOT11_VHT_AC
	if (WMODE_CAP(pAd->CommonCfg.PhyMode, WMODE_AC) &&
		(pAd->CommonCfg.Channel > 14) &&
		(pAd->hw_cfg.bbp_bw == BW_80) &&
		(pAd->CommonCfg.vht_bw == VHT_BW_80) &&
		(pAd->CommonCfg.vht_cent_ch != pAd->CommonCfg.CentralChannel)) {
		pAd->CommonCfg.vht_cent_ch = vht_cent_ch_freq(pAd, pAd->CommonCfg.Channel);
	}

	if (pAd->hw_cfg.bbp_bw == BW_80)
		pAd->hw_cfg.cent_ch = pAd->CommonCfg.vht_cent_ch;
	else
#endif /* DOT11_VHT_AC */
		pAd->hw_cfg.cent_ch = pAd->CommonCfg.CentralChannel;
		
	if (pAd->CommonCfg.BBPCurrentBW != pAd->hw_cfg.bbp_bw)
		bbp_set_bw(pAd, pAd->hw_cfg.bbp_bw);

	AsicSwitchChannel(pAd, pAd->hw_cfg.cent_ch, FALSE);
	AsicLockChannel(pAd, pAd->hw_cfg.cent_ch);

	ch = pAd->hw_cfg.cent_ch;

	switch(pAd->CommonCfg.BBPCurrentBW)
	{
		case BW_80:
			bw = 80;
			break;
		case BW_40:
			bw = 40;
			break;
		case BW_10:
			bw = 10;
			break;
		case BW_20:
		default:
			bw =20;
			break;
	}
	DBGPRINT(RT_DEBUG_TRACE, ("SYNC - End of SCAN, restore to %dMHz channel %d, Total BSS[%02d]\n",
				bw, ch, pAd->ScanTab.BssNr));
		

#ifdef CONFIG_AP_SUPPORT
	if (OpMode == OPMODE_AP)
	{
#ifdef APCLI_SUPPORT
#ifdef APCLI_AUTO_CONNECT_SUPPORT
			if ((pAd->ApCfg.ApCliAutoConnectRunning == TRUE)
#ifdef AP_PARTIAL_SCAN_SUPPORT
				&& (pAd->ApCfg.bPartialScanning == FALSE)
#endif /* AP_PARTIAL_SCAN_SUPPORT */
				)
			{
				if (!ApCliAutoConnectExec(pAd))
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Error in  %s\n", __FUNCTION__));
				}
			}			
#endif /* APCLI_AUTO_CONNECT_SUPPORT */
#endif /* APCLI_SUPPORT */
		pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;
		RTMPResumeMsduTransmission(pAd);

#ifdef CON_WPS
		if (pAd->conWscStatus != CON_WPS_STATUS_DISABLED)
		{
			MlmeEnqueue(pAd, AP_SYNC_STATE_MACHINE, APMT2_MLME_SCAN_COMPLETE, 0, NULL,0 );
			RTMP_MLME_HANDLER(pAd);
		}
#endif /* CON_WPS*/

		/* iwpriv set auto channel selection*/
		/* scanned all channels*/
		if (pAd->ApCfg.bAutoChannelAtBootup==TRUE)
		{
			pAd->CommonCfg.Channel = SelectBestChannel(pAd, pAd->ApCfg.AutoChannelAlg);
			pAd->ApCfg.bAutoChannelAtBootup = FALSE;
#ifdef DOT11_N_SUPPORT
			N_ChannelCheck(pAd);
#endif /* DOT11_N_SUPPORT */
			APStop(pAd);
			APStartUp(pAd);
		}

		if (((pAd->CommonCfg.Channel > 14) &&
			(pAd->CommonCfg.bIEEE80211H == TRUE) &&
			RadarChannelCheck(pAd, pAd->CommonCfg.Channel)) &&
			pAd->Dot11_H.RDMode != RD_SWITCHING_MODE)
		{
			if (pAd->Dot11_H.InServiceMonitorCount)
			{
				pAd->Dot11_H.RDMode = RD_NORMAL_MODE;
				AsicEnableBssSync(pAd);
			}
			else
			{
				pAd->Dot11_H.RDMode = RD_SILENCE_MODE;
			}
		}
		else
		{
			AsicEnableBssSync(pAd);
		}
	}

#ifdef APCLI_SUPPORT
#ifdef APCLI_CERT_SUPPORT
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
		if (APCLI_IF_UP_CHECK(pAd, 0) && pAd->bApCliCertTest == TRUE && ScanType == SCAN_2040_BSS_COEXIST)
		{
			UCHAR Status=1;
			DBGPRINT(RT_DEBUG_TRACE, ("@(%s)  Scan Done ScanType=%d\n", __FUNCTION__, ScanType));
			MlmeEnqueue(pAd, APCLI_CTRL_STATE_MACHINE, APCLI_CTRL_SCAN_DONE, 2, &Status, 0);			
		}
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
#endif /* APCLI_CERT_SUPPORT */		
#endif /* APCLI_SUPPORT */	
#endif /* CONFIG_AP_SUPPORT */

#ifdef CUSTOMER_DCC_FEATURE
	pAd->ChannelStats.LastReadTime = 0;
#endif
	return TRUE;
}



static INT scan_active(RTMP_ADAPTER *pAd, UCHAR OpMode, UCHAR ScanType)
{
	UCHAR *frm_buf = NULL;
	HEADER_802_11 Hdr80211;
	ULONG FrameLen = 0;
	UCHAR SsidLen = 0;


	if (MlmeAllocateMemory(pAd, &frm_buf) != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("SYNC - ScanNextChannel() allocate memory fail\n"));

#ifdef CONFIG_AP_SUPPORT
		if (OpMode == OPMODE_AP)
			pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;
#endif /* CONFIG_AP_SUPPORT */
		return FALSE;
	}

#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
	if (ScanType == SCAN_2040_BSS_COEXIST)
	{
		DBGPRINT(RT_DEBUG_INFO, ("SYNC - SCAN_2040_BSS_COEXIST !! Prepare to send Probe Request\n"));
	}
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */
	
	/* There is no need to send broadcast probe request if active scan is in effect.*/
	SsidLen = 0;
#ifndef APCLI_CONNECTION_TRIAL
	if ((ScanType == SCAN_ACTIVE) || (ScanType == FAST_SCAN_ACTIVE)
#ifdef WSC_STA_SUPPORT
		|| ((ScanType == SCAN_WSC_ACTIVE) && (OpMode == OPMODE_STA))
#endif /* WSC_STA_SUPPORT */
		)
		SsidLen = pAd->MlmeAux.SsidLen;
#endif /* APCLI_CONNECTION_TRIAL */

	{
#ifdef CONFIG_AP_SUPPORT
		/*IF_DEV_CONFIG_OPMODE_ON_AP(pAd) */
		if (OpMode == OPMODE_AP)
		{
#ifdef APCLI_SUPPORT
#ifdef WSC_INCLUDED
					if (ScanType == SCAN_WSC_ACTIVE) 
						MgtMacHeaderInitExt(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR, 
										pAd->ApCfg.ApCliTab[0].wdev.if_addr,
										BROADCAST_ADDR);	
					else
#endif /* WSC_INCLUDED */						
#endif /* APCLI_SUPPORT */		
			MgtMacHeaderInitExt(pAd, &Hdr80211, SUBTYPE_PROBE_REQ, 0, BROADCAST_ADDR,
								pAd->ApCfg.MBSSID[0].wdev.bssid,
								BROADCAST_ADDR);
		}
#endif /* CONFIG_AP_SUPPORT */

		MakeOutgoingFrame(frm_buf,               &FrameLen,
						  sizeof(HEADER_802_11),    &Hdr80211,
						  1,                        &SsidIe,
						  1,                        &SsidLen,
						  SsidLen,			        pAd->MlmeAux.Ssid,
						  1,                        &SupRateIe,
						  1,                        &pAd->CommonCfg.SupRateLen,
						  pAd->CommonCfg.SupRateLen,  pAd->CommonCfg.SupRate, 
						  END_OF_ARGS);

		if (pAd->CommonCfg.ExtRateLen)
		{
			ULONG Tmp;
			MakeOutgoingFrame(frm_buf + FrameLen,            &Tmp,
							  1,                                &ExtRateIe,
							  1,                                &pAd->CommonCfg.ExtRateLen,
							  pAd->CommonCfg.ExtRateLen,          pAd->CommonCfg.ExtRate, 
							  END_OF_ARGS);
			FrameLen += Tmp;
		}
	}
#ifdef DOT11_N_SUPPORT
	if (WMODE_CAP_N(pAd->CommonCfg.PhyMode))
	{
		ULONG	Tmp;
		UCHAR	HtLen;
		UCHAR	BROADCOM[4] = {0x0, 0x90, 0x4c, 0x33};
#ifdef RT_BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
#endif
		if (pAd->bBroadComHT == TRUE)
		{
			HtLen = pAd->MlmeAux.HtCapabilityLen + 4;
#ifdef RT_BIG_ENDIAN
			NdisMoveMemory(&HtCapabilityTmp, &pAd->MlmeAux.HtCapability, SIZE_HT_CAP_IE);
			*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
#ifdef UNALIGNMENT_SUPPORT
			{
				EXT_HT_CAP_INFO extHtCapInfo;

				NdisMoveMemory((PUCHAR)(&extHtCapInfo), (PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), sizeof(EXT_HT_CAP_INFO));
				*(USHORT *)(&extHtCapInfo) = cpu2le16(*(USHORT *)(&extHtCapInfo));
				NdisMoveMemory((PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), (PUCHAR)(&extHtCapInfo), sizeof(EXT_HT_CAP_INFO));		
			}
#else				
			*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = cpu2le16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));
#endif /* UNALIGNMENT_SUPPORT */

			MakeOutgoingFrame(frm_buf + FrameLen,          &Tmp,
							1,                                &WpaIe,
							1,                                &HtLen,
							4,                                &BROADCOM[0],
							pAd->MlmeAux.HtCapabilityLen,     &HtCapabilityTmp, 
							END_OF_ARGS);
#else
			MakeOutgoingFrame(frm_buf + FrameLen,          &Tmp,
							1,                                &WpaIe,
							1,                                &HtLen,
							4,                                &BROADCOM[0],
							pAd->MlmeAux.HtCapabilityLen,     &pAd->MlmeAux.HtCapability, 
							END_OF_ARGS);
#endif /* RT_BIG_ENDIAN */
		}
		else				
		{
			HtLen = sizeof(HT_CAPABILITY_IE);
#ifdef RT_BIG_ENDIAN
			NdisMoveMemory(&HtCapabilityTmp, &pAd->CommonCfg.HtCapability, SIZE_HT_CAP_IE);
			*(USHORT *)(&HtCapabilityTmp.HtCapInfo) = SWAP16(*(USHORT *)(&HtCapabilityTmp.HtCapInfo));
#ifdef UNALIGNMENT_SUPPORT
			{
				EXT_HT_CAP_INFO extHtCapInfo;

				NdisMoveMemory((PUCHAR)(&extHtCapInfo), (PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), sizeof(EXT_HT_CAP_INFO));
				*(USHORT *)(&extHtCapInfo) = cpu2le16(*(USHORT *)(&extHtCapInfo));
				NdisMoveMemory((PUCHAR)(&HtCapabilityTmp.ExtHtCapInfo), (PUCHAR)(&extHtCapInfo), sizeof(EXT_HT_CAP_INFO));		
			}
#else				
			*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo) = cpu2le16(*(USHORT *)(&HtCapabilityTmp.ExtHtCapInfo));
#endif /* UNALIGNMENT_SUPPORT */

			MakeOutgoingFrame(frm_buf + FrameLen,          &Tmp,
							1,                                &HtCapIe,
							1,                                &HtLen,
							HtLen,                            &HtCapabilityTmp, 
							END_OF_ARGS);
#else
			MakeOutgoingFrame(frm_buf + FrameLen,          &Tmp,
							1,                                &HtCapIe,
							1,                                &HtLen,
							HtLen,                            &pAd->CommonCfg.HtCapability, 
							END_OF_ARGS);
#endif /* RT_BIG_ENDIAN */
		}
		FrameLen += Tmp;

#ifdef DOT11N_DRAFT3
		if ((pAd->MlmeAux.Channel <= 14) && (pAd->CommonCfg.bBssCoexEnable == TRUE))
		{
			ULONG Tmp;
			HtLen = 1;
			MakeOutgoingFrame(frm_buf + FrameLen,            &Tmp,
							  1,					&ExtHtCapIe,
							  1,					&HtLen,
							  1,          			&pAd->CommonCfg.BSSCoexist2040.word, 
							  END_OF_ARGS);

			FrameLen += Tmp;
		}
#endif /* DOT11N_DRAFT3 */
	}
#endif /* DOT11_N_SUPPORT */

#ifdef APCLI_SUPPORT
#ifdef WSC_INCLUDED
			if ((ScanType == SCAN_WSC_ACTIVE) && (OpMode == OPMODE_AP))
			{
				BOOLEAN bHasWscIe = FALSE;
				/* 
					Append WSC information in probe request if WSC state is running
				*/
				if (pAd->ApCfg.ApCliTab[0].WscControl.bWscTrigger)
				{
					bHasWscIe = TRUE;
				}
#ifdef WSC_V2_SUPPORT
				else if (pAd->ApCfg.ApCliTab[0].WscControl.WscV2Info.bEnableWpsV2)
				{
					bHasWscIe = TRUE;	
				}
#endif /* WSC_V2_SUPPORT */

				if (bHasWscIe)
				{
					UCHAR		*pWscBuf = NULL, WscIeLen = 0;
					ULONG 		WscTmpLen = 0;

					os_alloc_mem(NULL, (UCHAR **)&pWscBuf, 512);
					if (pWscBuf != NULL)
					{
						NdisZeroMemory(pWscBuf, 512);
						WscBuildProbeReqIE(pAd, STA_MODE, pWscBuf, &WscIeLen);

						MakeOutgoingFrame(frm_buf + FrameLen,              &WscTmpLen,
										WscIeLen,                             pWscBuf,
										END_OF_ARGS);

						FrameLen += WscTmpLen;
						os_free_mem(NULL, pWscBuf);
					}
					else
						DBGPRINT(RT_DEBUG_WARN, ("%s:: WscBuf Allocate failed!\n", __FUNCTION__));
				}
			}
#endif /* WSC_INCLUDED */			
#endif /* APCLI_SUPPORT */

#ifdef DOT11_VHT_AC
	if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
		(pAd->MlmeAux.Channel > 14)) {		
		FrameLen += build_vht_ies(pAd, (UCHAR *)(frm_buf + FrameLen), SUBTYPE_PROBE_REQ);
	}
#endif /* DOT11_VHT_AC */

#ifdef WSC_STA_SUPPORT
	if (OpMode == OPMODE_STA)
	{
		BOOLEAN bHasWscIe = FALSE;
		/* 
			Append WSC information in probe request if WSC state is running
		*/
		if ((pAd->StaCfg.WscControl.WscEnProbeReqIE) && 
			(pAd->StaCfg.WscControl.WscConfMode != WSC_DISABLE) &&
			(pAd->StaCfg.WscControl.bWscTrigger == TRUE))
			bHasWscIe = TRUE;
#ifdef WSC_V2_SUPPORT
		else if ((pAd->StaCfg.WscControl.WscEnProbeReqIE) && 
			(pAd->StaCfg.WscControl.WscV2Info.bEnableWpsV2))
			bHasWscIe = TRUE;
#endif /* WSC_V2_SUPPORT */


		if (bHasWscIe)
		{
			UCHAR *pWscBuf = NULL, WscIeLen = 0;
			ULONG WscTmpLen = 0;

			os_alloc_mem(NULL, (UCHAR **)&pWscBuf, 512);
			if (pWscBuf != NULL)
			{
				NdisZeroMemory(pWscBuf, 512);
				WscBuildProbeReqIE(pAd, STA_MODE, pWscBuf, &WscIeLen);

				MakeOutgoingFrame(frm_buf + FrameLen,              &WscTmpLen,
								WscIeLen,                             pWscBuf,
								END_OF_ARGS);

				FrameLen += WscTmpLen;
				os_free_mem(NULL, pWscBuf);
			}
			else
				DBGPRINT(RT_DEBUG_WARN, ("%s:: WscBuf Allocate failed!\n", __FUNCTION__));
		}
	}

#endif /* WSC_STA_SUPPORT */



	MiniportMMRequest(pAd, 0, frm_buf, FrameLen);


	MlmeFreeMemory(pAd, frm_buf);

	return TRUE;
}

#ifdef WH_EZ_SETUP
#ifdef APCLI_SUPPORT
static void FireExtraProbeReq(RTMP_ADAPTER *pAd, UCHAR OpMode,
	UCHAR ScanType,
	struct wifi_dev *wdev,
	UCHAR *desSsid, UCHAR desSsidLen)
{
	UCHAR backSsid[MAX_LEN_OF_SSID];
	UCHAR backSsidLen = 0;

	NdisZeroMemory(backSsid, MAX_LEN_OF_SSID);

	//1. backup the original MlmeAux
	backSsidLen = pAd->ScanCtrl.SsidLen;
	NdisCopyMemory(backSsid, pAd->ScanCtrl.Ssid, backSsidLen);
	
	//2. fill the desried ssid into SM
	pAd->ScanCtrl.SsidLen = desSsidLen;
	NdisCopyMemory(pAd->ScanCtrl.Ssid, desSsid, desSsidLen);

	//3. scan action
	scan_active(pAd, OpMode, ScanType);

	//4. restore to ScanCtrl
	pAd->ScanCtrl.SsidLen  = backSsidLen;
	NdisCopyMemory(pAd->ScanCtrl.Ssid, backSsid, backSsidLen);
}
#endif /* APCLI_SUPPORT */
#endif

/*
	==========================================================================
	Description:
		Scan next channel
	==========================================================================
 */
VOID ScanNextChannel(RTMP_ADAPTER *pAd, UCHAR OpMode)
{
	UCHAR ScanType = pAd->MlmeAux.ScanType;
	UINT ScanTimeIn5gChannel = SHORT_CHANNEL_TIME;
	BOOLEAN ScanPending = FALSE;
	RALINK_TIMER_STRUCT *sc_timer = NULL;
	UINT stay_time = 0;
#ifdef WH_EZ_SETUP
	MAC_TABLE_ENTRY *pMacEntry;
#endif

#ifdef RALINK_ATE
	/* Nothing to do in ATE mode. */
	if (ATE_ON(pAd))
		return;
#endif /* RALINK_ATE */


	if ((pAd->MlmeAux.Channel == 0) || ScanPending) 
	{
		scan_ch_restore(pAd, OpMode);
#ifdef WH_EZ_SETUP
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
		if (IS_ADPTR_EZ_SETUP_ENABLED(pAd)) {
			pMacEntry = MacTableLookup(pAd, pAd->ApCfg.ApCliTab[BSS0].wdev.bssid);
			if(pMacEntry && pAd->ApCfg.ApCliTab[BSS0].Valid)
				ApCliRTMPSendNullFrame(pAd,pMacEntry->CurrTxRate, FALSE,
					pMacEntry, PWR_ACTIVE);
			if ((pAd->ApCfg.ScanReqwdev != NULL) &&
				pAd->ApCfg.ScanReqwdev->ez_driver_params.ez_scan == TRUE) {
				ez_handle_scan_channel_restore(pAd->ApCfg.ScanReqwdev);
				pAd->ApCfg.ScanReqwdev->ez_driver_params.ez_scan = FALSE;
				EZ_DEBUG(DBG_CAT_PROTO, DBG_SUBCAT_ALL, EZ_DBG_LVL_ERROR,
					("Start Join For  wdev-> type = %d wdev = %d\n", 
					pAd->ApCfg.ScanReqwdev->wdev_type,
					pAd->ApCfg.ScanReqwdev->wdev_idx));
				ApCliIfUp(pAd);
				pAd->ApCfg.ScanReqwdev = NULL;
			}
#ifdef MOBILE_APP_SUPPORT
			else if (IS_EZ_SETUP_ENABLED(&pAd->ApCfg.ApCliTab[BSS0].wdev)) {
				int i=0;
				unsigned char bssid[6] = {0};

				for (i = 0; i < pAd->ScanTab.BssNr; i++) {
					BSS_ENTRY *bss_entry = NULL;
					PAPCLI_STRUCT apcli_entry = &pAd->ApCfg.ApCliTab[BSS0];

					bss_entry = &pAd->ScanTab.BssEntry[i];
					if (bss_entry->support_easy_setup == TRUE)
						continue;
					if (SSID_EQUAL(apcli_entry->CfgSsid, apcli_entry->CfgSsidLen,
						bss_entry->Ssid, bss_entry->SsidLen)) {
						COPY_MAC_ADDR(bssid, bss_entry->Bssid);
					}
				}
				RtmpOSWrielessEventSend(pAd->net_dev, RT_WLAN_EVENT_CUSTOM,
					OID_WH_EZ_MAN_TRIBAND_SCAN_COMPLETE_EVENT,
					NULL, bssid, MAC_ADDR_LEN);
			}
		}
#endif			
#endif	//APCLI_SUPPORT
#endif	//CONFIG_AP_SUPPORT
#endif	// WH_EZ_SETUP

	} 
	else 
	{
#ifdef CONFIG_AP_SUPPORT
		if (OpMode == OPMODE_AP) {
			pAd->Mlme.ApSyncMachine.CurrState = AP_SCAN_LISTEN;
			}
#endif /* CONFIG_AP_SUPPORT */
#ifdef WH_EZ_SETUP
		if ((pAd->ApCfg.ScanReqwdev != NULL) &&
			IS_EZ_SETUP_ENABLED(pAd->ApCfg.ScanReqwdev) &&
			pAd->ApCfg.ScanReqwdev->ez_driver_params.scan_one_channel) {
			EZ_DEBUG(DBG_CAT_AP, DBG_SUBCAT_ALL, EZ_DBG_LVL_TRACE,
				("AP SYNC %s() - Only scan ch.%d and keep original channel setting.\n",
				__func__, pAd->ScanCtrl.Channel));
		} else
#endif /* WH_EZ_SETUP */
		{
			AsicSwitchChannel(pAd, pAd->MlmeAux.Channel, TRUE);
			AsicLockChannel(pAd, pAd->MlmeAux.Channel);
		}
		{
			BOOLEAN bScanPassive = FALSE;
			if (pAd->MlmeAux.Channel > 14)
			{
				if ((pAd->CommonCfg.bIEEE80211H == 1)
					&& RadarChannelCheck(pAd, pAd->MlmeAux.Channel))
					bScanPassive = TRUE;
			}
#ifdef CARRIER_DETECTION_SUPPORT
			if (pAd->CommonCfg.CarrierDetect.Enable == TRUE)
				bScanPassive = TRUE;
#endif /* CARRIER_DETECTION_SUPPORT */ 

			if (bScanPassive)
			{
				ScanType = SCAN_PASSIVE;
				ScanTimeIn5gChannel = MIN_CHANNEL_TIME;
			}
		}

		/* Check if channel if passive scan under current regulatory domain */
		if (CHAN_PropertyCheck(pAd, pAd->MlmeAux.Channel, CHANNEL_PASSIVE_SCAN) == TRUE)
			ScanType = SCAN_PASSIVE;


		if (OpMode == OPMODE_AP)
			sc_timer = &pAd->MlmeAux.APScanTimer;
		else
			sc_timer = &pAd->MlmeAux.ScanTimer;
			
		/* We need to shorten active scan time in order for WZC connect issue */
		/* Chnage the channel scan time for CISCO stuff based on its IAPP announcement */
		if (ScanType == FAST_SCAN_ACTIVE)
			stay_time = FAST_ACTIVE_SCAN_TIME;
		else /* must be SCAN_PASSIVE or SCAN_ACTIVE*/
		{

#ifdef CONFIG_AP_SUPPORT
			if ((OpMode == OPMODE_AP) && (pAd->ApCfg.bAutoChannelAtBootup))
				stay_time = AUTO_CHANNEL_SEL_TIMEOUT;
			else
#endif /* CONFIG_AP_SUPPORT */
			if (WMODE_CAP_2G(pAd->CommonCfg.PhyMode) &&
				WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
			{
				if (pAd->MlmeAux.Channel > 14)
				{
#ifdef CUSTOMER_DCC_FEATURE
					if(pAd->MlmeAux.ScanTime != 0)
					{
						stay_time = pAd->MlmeAux.ScanTime;
					}
					else
#endif
						stay_time = ScanTimeIn5gChannel;
				}
				else {
#ifdef CUSTOMER_DCC_FEATURE
					if(pAd->MlmeAux.ScanTime != 0)
					{
						stay_time = pAd->MlmeAux.ScanTime;
					}
					else
#endif
						stay_time = MIN_CHANNEL_TIME;
				}
			}
			else
#ifdef CUSTOMER_DCC_FEATURE
					if(pAd->MlmeAux.ScanTime != 0)
					{
						stay_time = pAd->MlmeAux.ScanTime;
					}
					else
#endif
				stay_time = MAX_CHANNEL_TIME;
		}
				
		RTMPSetTimer(sc_timer, stay_time);
			
		if (SCAN_MODE_ACT(ScanType))
		{
			if (scan_active(pAd, OpMode, ScanType) == FALSE) {
#ifdef WH_EZ_SETUP
				if((IS_EZ_SETUP_ENABLED(&pAd->ApCfg.ApCliTab[BSS0].wdev))
					&& (pAd->Mlme.ApSyncMachine.CurrState == AP_SCAN_LISTEN)) 
					pAd->Mlme.ApSyncMachine.CurrState = AP_SYNC_IDLE;
#endif

				return;
			}
#ifdef APCLI_SUPPORT
#ifdef WH_EZ_SETUP
			{
				PAPCLI_STRUCT pApCliEntry = NULL;
				UINT index = 0;

				if (IS_ADPTR_EZ_SETUP_ENABLED(pAd)) {
					for(index = 0; index < MAX_APCLI_NUM; index++)
					{
						pApCliEntry = &pAd->ApCfg.ApCliTab[index];
						if(pApCliEntry->CfgHideSsidLen > 0){
							FireExtraProbeReq(pAd,	OpMode, ScanType, &pApCliEntry->wdev,
								pApCliEntry->CfgHideSsid, pApCliEntry->CfgHideSsidLen);
						}
					}
				}
			}
#endif
#endif

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
#ifdef AP_PARTIAL_SCAN_SUPPORT
			if (pAd->ApCfg.bPartialScanning == TRUE)
			{
				/* Enhance Connectivity & for Hidden Ssid Scanning*/
				CHAR desiredSsid[MAX_LEN_OF_SSID], backSsid[MAX_LEN_OF_SSID];
				UCHAR desiredSsidLen, backSsidLen;

				desiredSsidLen= pAd->ApCfg.ApCliTab[0].CfgSsidLen;

				if (desiredSsidLen  > 0)
				{
					//printk("=====================>specific the %s scanning\n", pAd->ApCfg.ApCliTab[0].CfgSsid);
					/* 1. backup the original MlmeAux */
					backSsidLen = pAd->MlmeAux.SsidLen;
					NdisCopyMemory(backSsid, pAd->MlmeAux.Ssid, backSsidLen);
					
					/* 2. fill the desried ssid into SM */
					pAd->MlmeAux.SsidLen = desiredSsidLen;
					NdisCopyMemory(pAd->MlmeAux.Ssid, pAd->ApCfg.ApCliTab[0].CfgSsid, desiredSsidLen);

					/* 3. scan action */
					scan_active(pAd, OpMode, ScanType);
			
					/* 4. restore to MlmeAux */
					pAd->MlmeAux.SsidLen = backSsidLen;
					NdisCopyMemory(pAd->MlmeAux.Ssid, backSsid, backSsidLen);
				}
			}
#endif /* AP_PARTIAL_SCAN_SUPPORT */
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
		}

		/* For SCAN_CISCO_PASSIVE, do nothing and silently wait for beacon or other probe reponse*/
		
	}
}


BOOLEAN ScanRunning(RTMP_ADAPTER *pAd)
{
	BOOLEAN	rv = FALSE;

#ifdef CONFIG_AP_SUPPORT
#ifdef AP_SCAN_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			rv = ((pAd->Mlme.ApSyncMachine.CurrState == AP_SCAN_LISTEN) ? TRUE : FALSE);
#endif /* AP_SCAN_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	return rv;
}


#if defined(CONFIG_STA_SUPPORT) || defined(APCLI_SUPPORT)
#ifdef DOT11_N_SUPPORT
#ifdef DOT11N_DRAFT3
VOID BuildEffectedChannelList(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR		EChannel[11];
	UCHAR		i, j, k;
	UCHAR		UpperChannel = 0, LowerChannel = 0;
	
	RTMPZeroMemory(EChannel, 11);
	DBGPRINT(RT_DEBUG_TRACE, ("BuildEffectedChannelList:CtrlCh=%d,CentCh=%d,AuxCtrlCh=%d,AuxExtCh=%d\n", 
								pAd->CommonCfg.Channel, pAd->CommonCfg.CentralChannel, 
								pAd->MlmeAux.AddHtInfo.ControlChan, 
								pAd->MlmeAux.AddHtInfo.AddHtInfo.ExtChanOffset));

	/* 802.11n D4 11.14.3.3: If no secondary channel has been selected, all channels in the frequency band shall be scanned. */
	{
		for (k = 0;k < pAd->ChannelListNum;k++)
		{
			if (pAd->ChannelList[k].Channel <=14 )
			pAd->ChannelList[k].bEffectedChannel = TRUE;
		}
		return;
	}	
	
	i = 0;
	/* Find upper and lower channel according to 40MHz current operation. */
	if (pAd->CommonCfg.CentralChannel < pAd->CommonCfg.Channel)
	{
		UpperChannel = pAd->CommonCfg.Channel;
		LowerChannel = pAd->CommonCfg.CentralChannel-2;
	}
	else if (pAd->CommonCfg.CentralChannel > pAd->CommonCfg.Channel)
	{
		UpperChannel = pAd->CommonCfg.CentralChannel+2;
		LowerChannel = pAd->CommonCfg.Channel;
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("LinkUP 20MHz . No Effected Channel \n"));
		/* Now operating in 20MHz, doesn't find 40MHz effected channels */
		return;
	}

	DeleteEffectedChannelList(pAd);	

	DBGPRINT(RT_DEBUG_TRACE, ("BuildEffectedChannelList!LowerChannel ~ UpperChannel; %d ~ %d \n", LowerChannel, UpperChannel));

	/* Find all channels that are below lower channel.. */
	if (LowerChannel > 1)
	{
		EChannel[0] = LowerChannel - 1;
		i = 1;
		if (LowerChannel > 2)
		{
			EChannel[1] = LowerChannel - 2;
			i = 2;
			if (LowerChannel > 3)
			{
				EChannel[2] = LowerChannel - 3;
				i = 3;
			}
		}
	}
	/* Find all channels that are between  lower channel and upper channel. */
	for (k = LowerChannel;k <= UpperChannel;k++)
	{
		EChannel[i] = k;
		i++;
	}
	/* Find all channels that are above upper channel.. */
	if (UpperChannel < 14)
	{
		EChannel[i] = UpperChannel + 1;
		i++;
		if (UpperChannel < 13)
		{
			EChannel[i] = UpperChannel + 2;
			i++;
			if (UpperChannel < 12)
			{
				EChannel[i] = UpperChannel + 3;
				i++;
			}
		}
	}
	/* 
	    Total i channels are effected channels. 
	    Now find corresponding channel in ChannelList array.  Then set its bEffectedChannel= TRUE
	*/
	for (j = 0;j < i;j++)
	{
		for (k = 0;k < pAd->ChannelListNum;k++)
		{
			if (pAd->ChannelList[k].Channel == EChannel[j])
			{
				pAd->ChannelList[k].bEffectedChannel = TRUE;
				DBGPRINT(RT_DEBUG_TRACE,(" EffectedChannel[%d]( =%d)\n", k, EChannel[j]));
				break;
			}
		}
	}
}


VOID DeleteEffectedChannelList(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR		i;
	/*Clear all bEffectedChannel in ChannelList array. */
 	for (i = 0; i < pAd->ChannelListNum; i++)		
	{
		pAd->ChannelList[i].bEffectedChannel = FALSE;
	}	
}
#endif /* DOT11N_DRAFT3 */
#endif /* DOT11_N_SUPPORT */

VOID ScanParmFill(
	IN PRTMP_ADAPTER pAd,
	IN OUT MLME_SCAN_REQ_STRUCT *ScanReq,
	IN STRING Ssid[],
	IN UCHAR SsidLen,
	IN UCHAR BssType,
	IN UCHAR ScanType)
{
	NdisZeroMemory(ScanReq->Ssid, MAX_LEN_OF_SSID);
	ScanReq->SsidLen = SsidLen;
	NdisMoveMemory(ScanReq->Ssid, Ssid, SsidLen);
	ScanReq->BssType = BssType;
	ScanReq->ScanType = ScanType;
}
#endif /* defined(CONFIG_STA_SUPPORT) || defined(APCLI_SUPPORT) */


#endif /* SCAN_SUPPORT */

