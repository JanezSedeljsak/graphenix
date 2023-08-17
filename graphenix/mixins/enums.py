class FilterOperationEnum:
    EQUAL = 0
    NOTEQUAL = 1
    GREATER = 2
    GREATER_OR_EQUAL = 3
    LESS = 4
    LESS_OR_EQUAL = 5
    REGEX = 6
    IS_IN = 7
    NOT_IN = 8
    BETWEEN = 9
    IREGEX = 10

    @staticmethod
    def supports_index(operation):
        return operation in (
            FilterOperationEnum.EQUAL,
            FilterOperationEnum.IS_IN,
            FilterOperationEnum.BETWEEN
        )
    
class FieldTypeEnum:
    INT = 0
    STRING = 1
    BOOL = 2
    DATETIME = 3
    LINK = 4
    DOUBLE = 5
    VIRTUAL_LINK = 6