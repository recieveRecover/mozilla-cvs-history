
#define INC_SSL 1
#define INC_SMIME 1


#ifdef INC_SSL
extern int nss_InitLock;

extern void ATOB_AsciiToData();
extern void BTOA_DataToAscii();
extern void CERT_CertChainFromCert();
extern void CERT_CheckCertValidTimes();
extern void CERT_DestroyCertificate();
extern void CERT_DestroyCertificateList();
extern void CERT_DupCertList();
extern void CERT_DupCertificate();
extern void CERT_ExtractPublicKey();
extern void CERT_FindCertByName();
extern void CERT_FreeNicknames();
extern void CERT_GetCertNicknames();
extern void CERT_GetDefaultCertDB();
extern void CERT_GetSSLCACerts();
extern void CERT_NameToAscii();
extern void CERT_NewTempCertificate();
extern void CERT_VerifyCertName();
extern void CERT_VerifyCertNow();
extern void DER_Lengths();
extern void DSAU_DecodeDerSig();
extern void DSAU_EncodeDerSig();
extern void NSSRWLock_Destroy();
extern void NSSRWLock_HaveWriteLock();
extern void NSSRWLock_LockRead();
extern void NSSRWLock_LockWrite();
extern void NSSRWLock_New();
extern void NSSRWLock_UnlockRead();
extern void NSSRWLock_UnlockWrite();
extern void NSS_PutEnv();
extern void PK11_CipherOp();
extern void PK11_CloneContext();
extern void PK11_CreateContextByRawKey();
extern void PK11_CreateContextBySymKey();
extern void PK11_CreateDigestContext();
extern void PK11_Derive();
extern void PK11_DeriveWithFlags();
extern void PK11_DestroyContext();
extern void PK11_DigestBegin();
extern void PK11_DigestFinal();
extern void PK11_DigestKey();
extern void PK11_DigestOp();
extern void PK11_FindBestKEAMatch();
extern void PK11_FindCertFromNickname();
extern void PK11_FindFixedKey();
extern void PK11_FindKeyByAnyCert();
extern void PK11_FreeSlot();
extern void PK11_FreeSymKey();
extern void PK11_GenerateFortezzaIV();
extern void PK11_GenerateRandom();
extern void PK11_GetBestKeyLength();
extern void PK11_GetBestSlot();
extern void PK11_GetBestSlotMultiple();
extern void PK11_GetBestWrapMechanism();
extern void PK11_GetCurrentWrapIndex();
extern void PK11_GetInternalSlot();
extern void PK11_GetKeyData();
extern void PK11_GetMechanism();
extern void PK11_GetModuleID();
extern void PK11_GetPrivateModulusLen();
extern void PK11_GetSlotFromKey();
extern void PK11_GetSlotFromPrivateKey();
extern void PK11_GetSlotID();
extern void PK11_GetSlotSeries();
extern void PK11_GetTokenInfo();
extern void PK11_GetWindow();
extern void PK11_GetWrapKey();
extern void PK11_IVFromParam();
extern void PK11_IsPresent();
extern void PK11_KeyGen();
extern void PK11_MakeKEAPubKey();
extern void PK11_ParamFromIV();
extern void PK11_PubDecryptRaw();
extern void PK11_PubDerive();
extern void PK11_PubEncryptRaw();
extern void PK11_PubUnwrapSymKey();
extern void PK11_PubWrapSymKey();
extern void PK11_ReferenceSymKey();
extern void PK11_RestoreContext();
extern void PK11_SaveContext();
extern void PK11_SetFortezzaHack();
extern void PK11_SetWrapKey();
extern void PK11_Sign();
extern void PK11_SignatureLen();
extern void PK11_SymKeyFromHandle();
extern void PK11_TokenExists();
extern void PK11_UnwrapSymKey();
extern void PK11_UnwrapSymKeyWithFlags();
extern void PK11_Verify();
extern void PK11_VerifyKeyOK();
extern void PK11_WrapSymKey();
extern void PORT_Alloc();
extern void PORT_ArenaAlloc();
extern void PORT_ArenaZAlloc();
extern void PORT_Free();
extern void PORT_FreeArena();
extern void PORT_GetError();
extern void PORT_NewArena();
extern void PORT_Realloc();
extern void PORT_SetError();
extern void PORT_ZAlloc();
extern void PORT_ZFree();
extern void RSA_FormatBlock();
extern void SECITEM_CompareItem();
extern void SECITEM_CopyItem();
extern void SECITEM_FreeItem();
extern void SECITEM_ZfreeItem();
extern void SECKEY_CopyPrivateKey();
extern void SECKEY_CreateRSAPrivateKey();
extern void SECKEY_DestroyPrivateKey();
extern void SECKEY_DestroyPublicKey();
extern void SECKEY_PublicKeyStrength();
extern void SECKEY_UpdateCertPQG();
extern void SECMOD_LookupSlot();
extern void SECOID_GetAlgorithmTag();
extern void SGN_Begin();
extern void SGN_DestroyContext();
extern void SGN_End();
extern void SGN_NewContext();
extern void SGN_Update();
extern void VFY_Begin();
extern void VFY_CreateContext();
extern void VFY_DestroyContext();
extern void VFY_End();
extern void VFY_Update();

void
nss_referenceNSSFunctionsForSSL() {
	int tmp2 = nss_InitLock;

	ATOB_AsciiToData();
	BTOA_DataToAscii();
	CERT_CertChainFromCert();
	CERT_CheckCertValidTimes();
	CERT_DestroyCertificate();
	CERT_DestroyCertificateList();
	CERT_DupCertList();
	CERT_DupCertificate();
	CERT_ExtractPublicKey();
	CERT_FindCertByName();
	CERT_FreeNicknames();
	CERT_GetCertNicknames();
	CERT_GetDefaultCertDB();
	CERT_GetSSLCACerts();
	CERT_NameToAscii();
	CERT_NewTempCertificate();
	CERT_VerifyCertName();
	CERT_VerifyCertNow();
	DER_Lengths();
	DSAU_DecodeDerSig();
	DSAU_EncodeDerSig();
	NSSRWLock_Destroy();
	NSSRWLock_HaveWriteLock();
	NSSRWLock_LockRead();
	NSSRWLock_LockWrite();
	NSSRWLock_New();
	NSSRWLock_UnlockRead();
	NSSRWLock_UnlockWrite();
	NSS_PutEnv();
	PK11_CipherOp();
	PK11_CloneContext();
	PK11_CreateContextByRawKey();
	PK11_CreateContextBySymKey();
	PK11_CreateDigestContext();
	PK11_Derive();
	PK11_DeriveWithFlags();
	PK11_DestroyContext();
	PK11_DigestBegin();
	PK11_DigestFinal();
	PK11_DigestKey();
	PK11_DigestOp();
	PK11_FindBestKEAMatch();
	PK11_FindCertFromNickname();
	PK11_FindFixedKey();
	PK11_FindKeyByAnyCert();
	PK11_FreeSlot();
	PK11_FreeSymKey();
	PK11_GenerateFortezzaIV();
	PK11_GenerateRandom();
	PK11_GetBestKeyLength();
	PK11_GetBestSlot();
	PK11_GetBestSlotMultiple();
	PK11_GetBestWrapMechanism();
	PK11_GetCurrentWrapIndex();
	PK11_GetInternalSlot();
	PK11_GetKeyData();
	PK11_GetMechanism();
	PK11_GetModuleID();
	PK11_GetPrivateModulusLen();
	PK11_GetSlotFromKey();
	PK11_GetSlotFromPrivateKey();
	PK11_GetSlotID();
	PK11_GetSlotSeries();
	PK11_GetTokenInfo();
	PK11_GetWindow();
	PK11_GetWrapKey();
	PK11_IVFromParam();
	PK11_IsPresent();
	PK11_KeyGen();
	PK11_MakeKEAPubKey();
	PK11_ParamFromIV();
	PK11_PubDecryptRaw();
	PK11_PubDerive();
	PK11_PubEncryptRaw();
	PK11_PubUnwrapSymKey();
	PK11_PubWrapSymKey();
	PK11_ReferenceSymKey();
	PK11_RestoreContext();
	PK11_SaveContext();
	PK11_SetFortezzaHack();
	PK11_SetWrapKey();
	PK11_Sign();
	PK11_SignatureLen();
	PK11_SymKeyFromHandle();
	PK11_TokenExists();
	PK11_UnwrapSymKey();
	PK11_UnwrapSymKeyWithFlags();
	PK11_Verify();
	PK11_VerifyKeyOK();
	PK11_WrapSymKey();
	PORT_Alloc();
	PORT_ArenaAlloc();
	PORT_ArenaZAlloc();
	PORT_Free();
	PORT_FreeArena();
	PORT_GetError();
	PORT_NewArena();
	PORT_Realloc();
	PORT_SetError();
	PORT_ZAlloc();
	PORT_ZFree();
	RSA_FormatBlock();
	SECITEM_CompareItem();
	SECITEM_CopyItem();
	SECITEM_FreeItem();
	SECITEM_ZfreeItem();
	SECKEY_CopyPrivateKey();
	SECKEY_CreateRSAPrivateKey();
	SECKEY_DestroyPrivateKey();
	SECKEY_DestroyPublicKey();
	SECKEY_PublicKeyStrength();
	SECKEY_UpdateCertPQG();
	SECMOD_LookupSlot();
	SECOID_GetAlgorithmTag();
	SGN_Begin();
	SGN_DestroyContext();
	SGN_End();
	SGN_NewContext();
	SGN_Update();
	VFY_Begin();
	VFY_CreateContext();
	VFY_DestroyContext();
	VFY_End();
	VFY_Update();
}

#endif

#ifdef INC_SMIME
extern int CERT_IssuerAndSNTemplate;
extern int CERT_SetOfSignedCrlTemplate;
extern int SEC_PointerToAnyTemplate;
extern int SEC_PointerToOctetStringTemplate;
extern int SEC_SetOfAnyTemplate;

extern void CERT_CertListFromCert();
extern void CERT_DestroyCertArray();
extern void CERT_FindSMimeProfile();
extern void CERT_GetCertIssuerAndSN();
extern void CERT_ImportCerts();
extern void CERT_OpenCertDBFilename();
extern void CERT_SaveSMimeProfile();
extern void CERT_VerifyCert();
extern void DER_TimeToUTCTime();
extern void PK11_CreatePBEAlgorithmID();
extern void PK11_FindCertAndKeyByRecipientList();
extern void PK11_FindCertAndKeyByRecipientListNew();
extern void PK11_FortezzaHasKEA();
extern void PK11_FortezzaMapSig();
extern void PK11_GenerateNewParam();
extern void PK11_GetKeyStrength();
extern void PK11_IsHW();
extern void PK11_PBEKeyGen();
extern void PK11_ParamToAlgid();
extern void PK11_SetPasswordFunc();
extern void SEC_ASN1DecodeInteger();
extern void SEC_ASN1DecoderClearFilterProc();
extern void SEC_ASN1DecoderClearNotifyProc();
extern void SEC_ASN1DecoderFinish();
extern void SEC_ASN1DecoderSetFilterProc();
extern void SEC_ASN1DecoderSetNotifyProc();
extern void SEC_ASN1DecoderStart();
extern void SEC_ASN1DecoderUpdate();
extern void SEC_ASN1EncoderClearNotifyProc();
extern void SEC_ASN1EncoderClearStreaming();
extern void SEC_ASN1EncoderClearTakeFromBuf();
extern void SEC_ASN1EncoderFinish();
extern void SEC_ASN1EncoderSetNotifyProc();
extern void SEC_ASN1EncoderSetStreaming();
extern void SEC_ASN1EncoderSetTakeFromBuf();
extern void SEC_ASN1EncoderStart();
extern void SEC_ASN1EncoderUpdate();
extern void SEC_PKCS5IsAlgorithmPBEAlg();
extern void SEC_SignData();
extern void SGN_Digest();
extern void VFY_VerifyDigest();

nss_referenceNSSFunctionsForSMIME() {
	int tmp1=CERT_IssuerAndSNTemplate;
	int tmp2=CERT_SetOfSignedCrlTemplate;
	int tmp3=SEC_PointerToAnyTemplate;
	int tmp4=SEC_PointerToOctetStringTemplate;
	int tmp5=SEC_SetOfAnyTemplate;

	CERT_CertListFromCert();
	CERT_FindSMimeProfile();
	CERT_GetCertIssuerAndSN();
	CERT_ImportCerts();
	CERT_SaveSMimeProfile();
	CERT_VerifyCert();
	DER_TimeToUTCTime();
	PK11_CreatePBEAlgorithmID();
	PK11_FindCertAndKeyByRecipientListNew();
	PK11_FortezzaHasKEA();
	PK11_FortezzaMapSig();
	PK11_GenerateNewParam();
	PK11_GetKeyStrength();
	PK11_IsHW();
	PK11_ParamToAlgid();
	PK11_SetPasswordFunc();
	SEC_ASN1DecodeInteger();
	SEC_ASN1DecoderClearFilterProc();
	SEC_ASN1DecoderFinish();
	SEC_ASN1DecoderSetFilterProc();
	SEC_ASN1DecoderSetNotifyProc();
	SEC_ASN1DecoderStart();
	SEC_ASN1DecoderUpdate();
	SEC_ASN1EncoderClearNotifyProc();
	SEC_ASN1EncoderClearStreaming();
	SEC_ASN1EncoderClearTakeFromBuf();
	SEC_ASN1EncoderFinish();
	SEC_ASN1EncoderSetNotifyProc();
	SEC_ASN1EncoderSetStreaming();
	SEC_ASN1EncoderSetTakeFromBuf();
	SEC_ASN1EncoderStart();
	SEC_ASN1EncoderUpdate();
	SEC_PKCS5IsAlgorithmPBEAlg();
	SEC_SignData();
	SGN_Digest();
	VFY_VerifyDigest();
	CERT_OpenCertDBFilename();
	CERT_DestroyCertArray();
	PK11_PBEKeyGen();
	PK11_FindCertAndKeyByRecipientList();
	SEC_ASN1DecoderClearNotifyProc();
}
#endif

extern int CERT_CertificateRequestTemplate;

extern void CERT_DecodeCertificatePoliciesExtension();
extern void CERT_DecodeUserNotice();
extern void CERT_DestroyCertificatePoliciesExtension();
extern void CERT_GenTime2FormattedAscii();
extern void CERT_Hexify();
extern void DER_GeneralizedTimeToTime();
extern void HASH_GetHashObject();
extern void MD2_Flatten();
extern void MD2_Resurrect();
extern void MD5_Flatten();
extern void MD5_Resurrect();
extern void NSSBase64Decoder_Create();
extern void NSSBase64Decoder_Destroy();
extern void NSSBase64Decoder_Update();
extern void NSSBase64Encoder_Create();
extern void NSSBase64Encoder_Destroy();
extern void NSSBase64Encoder_Update();
extern void PK11_ChangePW();
extern void PK11_CheckUserPassword();
extern void PK11_DoPassword();
extern void PK11_FindKeyByKeyID();
extern void PK11_InitPin();
extern void PK11_NeedUserInit();
extern void PQG_ParamGen();
extern void PQG_VerifyParams();
extern void SECITEM_ReallocItem();
extern void SECKEY_DeriveKeyDBPassword();
extern void SECKEY_GetKeyDBVersion();

nss_CMDExports() {
	int tmp1 = CERT_CertificateRequestTemplate;

	CERT_DecodeCertificatePoliciesExtension();
	CERT_DecodeUserNotice();
	CERT_DestroyCertificatePoliciesExtension();
	CERT_GenTime2FormattedAscii();
	CERT_Hexify();
	DER_GeneralizedTimeToTime();
	HASH_GetHashObject();
	MD2_Flatten();
	MD2_Resurrect();
	MD5_Flatten();
	MD5_Resurrect();
	NSSBase64Decoder_Create();
	NSSBase64Decoder_Destroy();
	NSSBase64Decoder_Update();
	NSSBase64Encoder_Create();
	NSSBase64Encoder_Destroy();
	NSSBase64Encoder_Update();
	PK11_ChangePW();
	PK11_CheckUserPassword();
	PK11_DoPassword();
	PK11_FindKeyByKeyID();
	PK11_InitPin();
	PK11_NeedUserInit();
	PQG_ParamGen();
	PQG_VerifyParams();
	SECITEM_ReallocItem();
	SECKEY_DeriveKeyDBPassword();
	SECKEY_GetKeyDBVersion();
}

