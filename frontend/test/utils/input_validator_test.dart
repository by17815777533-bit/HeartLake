import 'package:flutter_test/flutter_test.dart';
import 'package:heart_lake/utils/input_validator.dart';

void main() {
  group('InputValidator.validateUUID', () {
    test('accepts standard UUID', () {
      const id = '3990a48f-83ac-4bd0-8643-11ebb5876a64';
      expect(InputValidator.validateUUID(id, '石头ID'), id);
    });

    test('accepts prefixed UUID style IDs', () {
      const id = 'stone_3990a48f-83ac-4bd0-8643-11ebb5876a64';
      expect(InputValidator.validateUUID(id, '石头ID'), id);
    });

    test('accepts common business IDs', () {
      const id = 'anonymous_933b49a3ca09';
      expect(InputValidator.validateUUID(id, '用户ID'), id);
    });

    test('rejects path traversal input', () {
      expect(
        () => InputValidator.validateUUID('../etc/passwd', '石头ID'),
        throwsA(isA<ValidationException>()),
      );
    });

    test('rejects slash-separated path input', () {
      expect(
        () => InputValidator.validateUUID('stone/123', '石头ID'),
        throwsA(isA<ValidationException>()),
      );
    });

    test('rejects malformed ID', () {
      expect(
        () => InputValidator.validateUUID('stone id with spaces', '石头ID'),
        throwsA(isA<ValidationException>()),
      );
    });
  });
}
