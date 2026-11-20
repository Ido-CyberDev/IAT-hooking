סוג נפוץ של Hooking המסתמך על המבנה של הפורמט PE . 

בוא נבין אתה תאוריה ואז נצלול למימוש:

בwindows יש כל קבצי ההרצה בעצם בוניים בכך שעל הדיסק הם מכילים את כל המידע שהם צריכים על מנת שהLoder יוכל לטעון אותם לזיכרון בצורה שתאפשר להם לרוץ, אחד התלויות של קובץ הרצה לרוץ זה הקבצים שמהם הוא עושה Import (כמובן בצורה סטטית מה שקורה בזמן ריצה לא ידוע כאשר הקובץ נמצא על הדיסק). 

נדון באיך הדבר הזה קורה ואיך לנצל את אותו לטובתינו:

אז איך בכלל זה נראה?
נסתכל על PE רנדומלי שיש לי על המחשב. 

## Import Directory

![[Pasted image 20261120161058.png]]

![[Pasted image 20261120161151.png]]


ניתן לראות שלא רק שרואים באיזה dllים בקובץ שלי משתמש אלא גם באיזה פונקציות מתוכם הוא עושה שימוש!.

למה בעצם? 
הloader תפקידו הוא לטעון את הPE האחרים שנעשה בהם שימוש לזיכרון, לאחר מכן הוא אחראי על למצוא את הכתובות של הפונקיות האלה (לפי הexport table שלהם, ולאחר הוא מעדכן את הכתובות שלהם בimport address table בשביל השימוש בהם).

לכל Dll יש רשומה בטבלה הזאת של הdllים שהpe שלנו משתמש בהם, לכל אחד מהם יש שימוש בstruct:

```
typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        DWORD   Characteristics;
        DWORD   OriginalFirstThunk;
    } DUMMYUNIONNAME;
    DWORD   TimeDateStamp;
    DWORD   ForwarderChain;
    DWORD   Name;
    DWORD   FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_DESCRIPTOR UNALIGNED *PIMAGE_IMPORT_DESCRIPTOR;
```

הmember המעניין שאותנו הוא OriginalFirstThunk שהוא מחזיק את הRVA אל הILT.

## Import Lookup Table

לכל Dll שאנחנו עושים לו import שי את הlookup table שלו. הstruct הזה אומר לloader לכל dll איזה פונקציות נדרשות ממנו. 

```
typedef struct _IMAGE_IMPORT_BY_NAME {
    WORD    Hint;
    CHAR   Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;
```

## Import Address Table

בעת השהייה על הדיסק נמצא שם אותו מידע כמו הINT אבל בעת הטעינה לזיכרון יש שם את הכתובות של הפונקציות עצמם מכיוון שהloader שם אותם שם. 


```
typedef struct _IMAGE_THUNK_DATA64 {
    union {
        ULONGLONG ForwarderString;
        ULONGLONG Function;
        ULONGLONG Ordinal;
        ULONGLONG AddressOfData;
    } u1;
} IMAGE_THUNK_DATA64;
```




עכשיו לחלק המעניין בואו נעשה hook כלומר נכתוב תוכנית שטוענת dll שיש לו Import table לאחר מכן על פונקציה מסויימת בזיכון נמצאע אותה נחליף את הכתובת שלה במבנה לפונקציה שלנו ונראה מה קורה.


1. נבנה בעצמינו dll שמשתמש בפונקציה שנמצאת בuser32.dll על מנת שיהיה לו Import directory table. 
2. נטען אותו דינאמית אלינו בפרוייקט שלנו
3. נפרסר את המבני נתונים הרלוונטים בזיכרון של הdll שטענו ולפי התוכנה cff explorer נראה במה הוא משתמש ונעשה לזה hook.
4. נשנה את הIAT על מנת להכווין לפונקציה שאנחנו רוצים.
5. נקרא לפונקציה שהdll מייחצן באמצעות  getProccAddress.
   
   הפונקציה שלנו תקרא 