### Unikey AS ###

Unikey AS - The auto switching mode English/Vietnamese for [Unikey NT](https://www.unikey.org/)

Tool này có chức năng rất đơn giản là tự động chuyển kiểu gõ EN/VN cho riêng mỗi app lúc focusing dựa vào quy định theo `process name`, `class name`, `title name` trong file cài đặt `Unikey.AS.INI`, rất tiện và không còn bực mình vì kiểu gõ trong khi làm việc coding, chatting, etc.

#### Chức năng
- [x] Tự động chuyển chế độ gõ EN/VI cho mỗi ứng dụng theo quy định của người dùng
- [x] Hỗ trợ tất cả các phiên bản của Unikey NT (hỗ trợ cho cả 32-bit & 64-bit)

#### Cài đặt
<details>
<summary>Nhấn để xem ...</summary>

> 1. Tải một phiên bản trên trang web chính thức [unikey.org](https://www.unikey.org/) về giải nén và chạy.
>
> 2. Tải tool [Unikey AS](https://github.com/vic4key/Unikey-AS/releases) mới nhất về giải nén và chạy.
>
> 3. Thêm/Xóa app vào file cài đặt bằng nút ... Để biết được `process name`, `class name`, `title name` thì focus vào app muốn set, sử dụng một trong ba tham số này, và set giá trị `0 (EN)`, `1 (VI)`. Ví dụ: Muốn set process `Visual Studio` luôn sử dụng tiếng anh thì `devenv.exe=0`.
>
> 4. Ngoài ra
>> - Force Mode : Nếu tích chọn thì người dùng không thể chuyển kiểu gõ khác. Nếu bỏ tích chọn thì người dùng có thể linh động chuyển kiểu gõ lúc đang focus nhưng lần sau focus lại app thì tool vẫn sẽ tự động chuyển kiểu gõ theo cài đặt.
>> - Top Most : Hiển thị GUI tool trên các app khác.
</details>

#### Minh họa

![](Screenshots/Unikey-AS.PNG)

![](Screenshots/Unikey-AS.INI.PNG)

#### Contact
Feel free to contact via [Twitter](https://twitter.com/vic4key) / [Gmail](mailto:vic4key@gmail.com) / [Blog](https://blog.vic.onl/) / [Website](https://vic.onl/)