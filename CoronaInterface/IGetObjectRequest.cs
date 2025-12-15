using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{
    public interface IGetObjectRequest : ICoronaBaseRequest
    {
        public string ClassName { get; set; }
        public long ObjectId { get; set; }
    }

    public class GetObjectRequest : CoronaBaseRequest, IGetObjectRequest
    {
        public string ClassName { get; set; } = string.Empty;
        public long ObjectId { get; set; } = 0;
    }
}
