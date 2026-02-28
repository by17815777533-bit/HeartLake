// E2E 端到端加密工具 - X25519 密钥交换 + AES-256-GCM 加密
import 'dart:convert';
import 'dart:typed_data';
import 'package:cryptography/cryptography.dart';

/// 端到端加密工具，保护用户间私密通信
///
/// 加密流程：
/// 1. 双方各自生成 X25519 密钥对，交换公钥
/// 2. 通过 ECDH 派生 256-bit 共享密钥
/// 3. 使用 AES-256-GCM 对消息进行认证加密
///
/// 密文格式为 base64(nonce[12] + ciphertext + mac[16])，
/// 也支持分离字段格式（ciphertext/iv/tag 各自独立 base64）。
class E2EEncryption {
  SecretKey? _sharedSecret;

  /// 生成 X25519 密钥对
  Future<SimpleKeyPair> generateKeyPair() async {
    final algorithm = X25519();
    return await algorithm.newKeyPair();
  }

  /// 导出公钥为 base64
  Future<String> exportPublicKey(SimpleKeyPair keyPair) async {
    final publicKey = await keyPair.extractPublicKey();
    return base64Encode(publicKey.bytes);
  }

  /// 从对方公钥派生共享密钥
  Future<void> deriveSharedSecret(
    SimpleKeyPair myKeyPair,
    String peerPublicKeyBase64,
  ) async {
    final algorithm = X25519();
    final peerPublicKey = SimplePublicKey(
      base64Decode(peerPublicKeyBase64),
      type: KeyPairType.x25519,
    );
    _sharedSecret = await algorithm.sharedSecretKey(
      keyPair: myKeyPair,
      remotePublicKey: peerPublicKey,
    );
  }

  /// AES-256-GCM 加密，返回 base64(nonce + ciphertext + mac)
  Future<String> encrypt(String plaintext) async {
    if (_sharedSecret == null) throw StateError('共享密钥未建立');
    final algorithm = AesGcm.with256bits();
    final secretBox = await algorithm.encrypt(
      utf8.encode(plaintext),
      secretKey: _sharedSecret!,
    );
    final combined = Uint8List.fromList([
      ...secretBox.nonce,
      ...secretBox.cipherText,
      ...secretBox.mac.bytes,
    ]);
    return base64Encode(combined);
  }

  /// AES-256-GCM 解密
  Future<String> decrypt(String encrypted) async {
    if (_sharedSecret == null) throw StateError('共享密钥未建立');
    final algorithm = AesGcm.with256bits();
    final bytes = base64Decode(encrypted);
    // nonce: 12 bytes, mac: 16 bytes, ciphertext: 中间部分
    final nonce = bytes.sublist(0, 12);
    final mac = Mac(bytes.sublist(bytes.length - 16));
    final cipherText = bytes.sublist(12, bytes.length - 16);
    final secretBox = SecretBox(cipherText, nonce: nonce, mac: mac);
    final decrypted = await algorithm.decrypt(
      secretBox,
      secretKey: _sharedSecret!,
    );
    return utf8.decode(decrypted);
  }

  /// 解密分离字段格式（ciphertext + iv + tag，均为base64）
  Future<String> decryptEnvelope({
    required String ciphertextBase64,
    required String ivBase64,
    required String tagBase64,
  }) async {
    if (_sharedSecret == null) throw StateError('共享密钥未建立');
    final algorithm = AesGcm.with256bits();
    final nonce = base64Decode(ivBase64);
    final cipherText = base64Decode(ciphertextBase64);
    final mac = Mac(base64Decode(tagBase64));
    final secretBox = SecretBox(cipherText, nonce: nonce, mac: mac);
    final decrypted = await algorithm.decrypt(
      secretBox,
      secretKey: _sharedSecret!,
    );
    return utf8.decode(decrypted);
  }

  /// 共享密钥是否已建立，加密/解密前必须为 true
  bool get isReady => _sharedSecret != null;

  /// 释放共享密钥，切换聊天对象时应调用
  void dispose() {
    _sharedSecret = null;
  }
}
